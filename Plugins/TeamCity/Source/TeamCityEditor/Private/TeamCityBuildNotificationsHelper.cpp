#include "TeamCityBuildNotificationsHelper.h"
#include "TeamCityWebAPI.h"
#include "TeamCityStyle.h"
#include "TeamCityEditorPCH.h"

#include "Editor/MainFrame/Public/Interfaces/IMainFrameModule.h"
#include "Editor/EditorPerProjectUserSettings.h"

#define LOCTEXT_NAMESPACE "TeamCity"

FTeamCityBuildNotification::FTeamCityBuildNotification()
	: BuildID("")
	, HyperlinkURL("")
	, TimeElapsedSinceLastQuery(0.0f)
	, QueryInterval(10.0f)
	, bIsTickEnabled(true)
{}

void FTeamCityBuildNotification::Tick(float DeltaTime)
{
	TimeElapsedSinceLastQuery += DeltaTime;

	if (TimeElapsedSinceLastQuery >= QueryInterval)
	{
		QueryTeamCityPersonalBuildStatus();
	}
}

TStatId FTeamCityBuildNotification::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FTeamCityBuildNotification, STATGROUP_Tickables);
}

void FTeamCityBuildNotification::ResolveTeamCityBuildNotification(const FText& InText, SNotificationItem::ECompletionState InCompletionState)
{
	if (NotificationItemPtr.IsValid())
	{		
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("NotificationText"), InText);
		Arguments.Add(TEXT("WebURL"), FText::FromString(HyperlinkURL));
		FText LogText(FText::Format(LOCTEXT("TeamCityPersonalBuildFailureNotification", "{NotificationText} See TeamCity build log: {WebURL}"), Arguments));
		
		switch (InCompletionState)
		{
		case SNotificationItem::CS_Success:
			UE_LOG(LogTeamCityEditor, Display, TEXT("%s"), *LogText.ToString());
			GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileSuccess_Cue.CompileSuccess_Cue"));
			break;
		case SNotificationItem::CS_Fail:
			UE_LOG(LogTeamCityEditor, Error, TEXT("%s"), *LogText.ToString());
			GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileFailed_Cue.CompileFailed_Cue"));
			break;
		default:
			UE_LOG(LogTeamCityEditor, Warning, TEXT("%s"), *LogText.ToString());
			GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileFailed_Cue.CompileFailed_Cue"));
			break;
		}

		TSharedPtr<SNotificationItem> NotificationItem = NotificationItemPtr.Pin();
		NotificationItem->SetText(InText);
		NotificationItem->SetExpireDuration(3.0f);
		NotificationItem->SetFadeOutDuration(0.5f);
		NotificationItem->SetCompletionState(InCompletionState);
		NotificationItem->ExpireAndFadeout();

		// Since the task was probably fairly long, we should try and grab the users attention if they have the option enabled.
		const UEditorPerProjectUserSettings* SettingsPtr = GetDefault<UEditorPerProjectUserSettings>();
		if (SettingsPtr->bGetAttentionOnUATCompletion)
		{
			IMainFrameModule* MainFrame = FModuleManager::LoadModulePtr<IMainFrameModule>("MainFrame");
			if (MainFrame != nullptr)
			{
				TSharedPtr<SWindow> ParentWindow = MainFrame->GetParentWindow();
				if (ParentWindow != nullptr)
				{
					ParentWindow->DrawAttention(FWindowDrawAttentionParameters(EWindowDrawAttentionRequestType::UntilActivated));
				}
			}
		}
	}

	return;
}

void FTeamCityBuildNotification::OnReceiveBuildLog(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	const bool GotValidResponse = bSucceeded && HttpResponse->GetResponseCode() == 200;
	if (GotValidResponse)
	{
		const FString BuildLog(HttpResponse->GetContentAsString());

		// @RossB: TODO: Write log to separate tab
		// UE_LOG(LogTeamCityEditor, Display, TEXT("%s"), *BuildLog.RightChop(TeamCityBuildLog.Len()));
		TeamCityBuildLog = BuildLog;
	}
}

void FTeamCityBuildNotification::OnCanceledPersonalBuild(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	static const FText TaskName = FText::FromString("TeamCity Personal Build");
	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("TaskName"), TaskName);
	const FText Text(FText::Format(LOCTEXT("TeamCityPersonalBuildCanceledNotification", "{TaskName} canceled!"), Arguments));

	ResolveTeamCityBuildNotification(Text, SNotificationItem::CS_None);
}

void FTeamCityBuildNotification::OnPersonalBuildFailure()
{
	static const FText TaskName = FText::FromString("TeamCity Personal Build");
	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("TaskName"), TaskName);
	const FText Text(FText::Format(LOCTEXT("TeamCityPersonalBuildFailureNotification", "{TaskName} failed!"), Arguments));

	ResolveTeamCityBuildNotification(Text, SNotificationItem::CS_Fail);
}

void FTeamCityBuildNotification::OnPersonalBuildSuccessful()
{
	static const FText TaskName = FText::FromString("TeamCity Personal Build");
	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("TaskName"), TaskName);
	const FText Text(FText::Format(LOCTEXT("TeamCityPersonalBuildFailureNotification", "{TaskName} complete!"), Arguments));

	ResolveTeamCityBuildNotification(Text, SNotificationItem::CS_Success);
}

void FTeamCityBuildNotification::OnQueryTeamCityPersonalBuildStatus(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	// Get the build log from TeamCity and print it to the UE4 log
	const FString Request(TEXT("downloadBuildLog.html?buildId=") + BuildID);
	FTeamCityWebAPI::PerformGetRequestRaw(Request, this, &FTeamCityBuildNotification::OnReceiveBuildLog);

	const bool GotValidResponse = bSucceeded && HttpResponse->GetResponseCode() == 200;
	if (GotValidResponse)
	{
		FXmlFile XmlFile;
		const bool XmlParseSuccess = FTeamCityWebAPI::ParseTeamCityResponse(XmlFile, HttpResponse);
		if (XmlParseSuccess)
		{
			const FXmlNode* RootNode = XmlFile.GetRootNode();
			const FString BuildState(RootNode->GetAttribute(TEXT("state")));

			if (BuildState == "finished")
			{
				const FString BuildStatus(RootNode->GetAttribute(TEXT("status")));
				if (BuildStatus == "SUCCESS")
				{
					OnPersonalBuildSuccessful();
				}
				else if(BuildStatus == "UNKNOWN")
				{
					OnCanceledPersonalBuild(HttpRequest, HttpResponse, bSucceeded);
				}
				else if (BuildStatus == "FAILURE")
				{
					OnPersonalBuildFailure();				
				}
				bIsTickEnabled = false;
			}
		}
	}
}

void FTeamCityBuildNotification::QueryTeamCityPersonalBuildStatus()
{
	FTeamCityWebAPI::PerformGetRequestRaw(TEXT("app/rest/builds/id:") + BuildID, this, &FTeamCityBuildNotification::OnQueryTeamCityPersonalBuildStatus);
	TimeElapsedSinceLastQuery = 0.0f;
}

static void OnTeamCityBuildCanceled(TSharedPtr<FTeamCityBuildNotification> TeamCityBuildNotification)
{
	// Sync with TeamCity
	TeamCityBuildNotification->QueryTeamCityPersonalBuildStatus();

	FString CancelConfiguration(TEXT("\
		<buildCancelRequest comment='<comment>' readdIntoQueue='false' />\
	"));

	CancelConfiguration.ReplaceInline(TEXT("<comment>"), TEXT("Build canceled via UE4."));

	const FTCHARToUTF8 ANSIString(*CancelConfiguration);
	const ANSICHAR* ANSIBytes = ANSIString.Get();
	const int32 ANSIByteSize = ANSIString.Length();
	TArray<uint8> Payload;
	Payload.Append((uint8*)ANSIBytes, ANSIByteSize * sizeof(ANSICHAR));

	FTeamCityWebAPI::PerformPostRequest(TEXT("app/rest/builds/id:") + TeamCityBuildNotification->BuildID, TEXT("application/xml"), Payload, TeamCityBuildNotification.Get(), &FTeamCityBuildNotification::OnCanceledPersonalBuild);
}

FTeamCityBuildNotificationsHelper::FTeamCityBuildNotificationsHelper()
{}

FTeamCityBuildNotificationsHelper::~FTeamCityBuildNotificationsHelper()
{}

void FTeamCityBuildNotificationsHelper::AddTeamCityBuildNotification(const FText& NotificationText, const FString& HyperlinkURL, const FString& BuildID)
{
	GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileStart_Cue.CompileStart_Cue"));

	TSharedPtr<FTeamCityBuildNotification> TeamCityBuildNotification = MakeShareable(new FTeamCityBuildNotification());
	TeamCityBuildNotification->BuildID = BuildID;

	FNotificationInfo Info(NotificationText);
	TSharedPtr<SNotificationItem> NotificationItem;

	Info.Image = FTeamCityStyle::Get()->GetBrush("TeamCity.Icon.BuildNotification");
	Info.bFireAndForget = false;
	Info.FadeOutDuration = 0.0f;
	Info.ExpireDuration = 0.0f;
	if (HyperlinkURL.Len() > 0)
	{
		Info.Hyperlink = FSimpleDelegate::CreateStatic(&FTeamCityEditorModule::OpenHyperlink, HyperlinkURL);
		Info.HyperlinkText = LOCTEXT("ShowOutputLogHyperlink", "Show TeamCity Build Log");
	}
	Info.ButtonDetails.Add(
		FNotificationButtonInfo(
			LOCTEXT("UatTaskCancel", "Cancel"),
			LOCTEXT("UatTaskCancelToolTip", "Cancels execution of this task."),
			FSimpleDelegate::CreateStatic(&OnTeamCityBuildCanceled, TeamCityBuildNotification),
			SNotificationItem::CS_Pending
		)
	);

	NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
	NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);

	TeamCityBuildNotification->NotificationItemPtr = NotificationItem;
	TeamCityBuildNotification->HyperlinkURL = HyperlinkURL;
}

#undef LOCTEXT_NAMESPACE