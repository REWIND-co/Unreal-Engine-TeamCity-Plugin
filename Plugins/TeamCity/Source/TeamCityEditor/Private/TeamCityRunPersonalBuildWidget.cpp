#include "TeamCityRunPersonalBuildWidget.h"
#include "TeamCityStyle.h"
#include "TeamCityEditorPCH.h"
#include "TeamCityBuildNotificationsHelper.h"
#include "TeamCityWebAPI.h"
#include "TeamCityFileSelectWidget.h"
#include "TeamCityFileItem.h"
#include "TeamCityEditor.h"
#include "HAL/PlatformFilemanager.h"

#if WITH_EDITOR
#include "Widgets/Views/SListView.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Images/SThrobber.h"
#endif

#define LOCTEXT_NAMESPACE "TeamCity"

ECheckBoxState FRunPersonalBuildItem::GetCheckState() const
{
	return CheckState;
}

void FRunPersonalBuildItem::SetCheckState(ECheckBoxState State)
{
	SetCheckState_RecursiveUp(State);
	SetCheckState_RecursiveDown(State);
}

void FRunPersonalBuildItem::SetCheckState_RecursiveUp(ECheckBoxState State)
{
	CheckState = State;

	// update the parent check state
	if (Parent.IsValid())
	{
		bool AllChildrenMatch = true;
		for (auto& Child : Parent->Children)
		{
			if (Child.Get() != this && Child->GetCheckState() != State) { AllChildrenMatch = false; break; }
		}

		ECheckBoxState ParentState = ECheckBoxState::Undetermined;
		if (AllChildrenMatch) { ParentState = State; }
		Parent->SetCheckState_RecursiveUp(ParentState);
	}
}

void FRunPersonalBuildItem::SetCheckState_RecursiveDown(ECheckBoxState State)
{
	CheckState = State;

	// update children state
	for (auto& Child : Children) { Child->SetCheckState_RecursiveDown(CheckState); }
}

void STeamCityRunPersonalBuildWidget::Construct(const FArguments& InArgs)
{
	ParentWindowPtr = InArgs._ParentWindow;
	FileSelectWidget = InArgs._FileSelectWidget;
	IsProcessingPersonalBuild = false;

#if WITH_EDITOR
	ChildSlot
	[
		SNew(SBorder)
		.HAlign(HAlign_Fill)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.FillHeight(6.0f)
			.Padding(0.0f, 6.0f, 0.0f, 0.0f)
			[
				SNew(SBorder)
				.HAlign(HAlign_Fill)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
				[
					SAssignNew(BuildConfigTreeView, STreeView<RunPersonalBuildItemPtr>)
						.TreeItemsSource(&Items)
						.OnGenerateRow(this, &STeamCityRunPersonalBuildWidget::OnGenerateRow)
						.OnGetChildren(this, &STeamCityRunPersonalBuildWidget::OnGetChildren)
						.SelectionMode(ESelectionMode::None)
						.OnSetExpansionRecursive(this, &STeamCityRunPersonalBuildWidget::SetTreeItemExpansionRecursive)
				]
			]
			+SVerticalBox::Slot()
			.Padding(8.0f, 4.0f, 8.0f, 2.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.VAlign(VAlign_Top)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("TeamCityPersonalBuildDescription_Label", "Description"))
				]
				+ SVerticalBox::Slot()
				.VAlign(VAlign_Top)
				.FillHeight(2.0f)
				[
					SNew(SMultiLineEditableTextBox)
					.ToolTipText(LOCTEXT("TeamCityPersonalBuildDescription_Tooltip", "Should describe the intent of this personal build."))
					.SelectAllTextWhenFocused(true)
					.AutoWrapText(true)
					.AlwaysShowScrollbars(true)
					.OnTextCommitted(this, &STeamCityRunPersonalBuildWidget::SetDescription)
					.OnTextChanged(this, &STeamCityRunPersonalBuildWidget::SetDescription, ETextCommit::Default)
				]
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Bottom)
			.Padding(8.0f, 16.0f, 8.0f, 8.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.5f)
				.HAlign(HAlign_Left)
				[
					SNew(SThrobber)
					.Visibility(this, &STeamCityRunPersonalBuildWidget::GetProcessingPersonalBuildThrobberVisibility)
				]
				+SHorizontalBox::Slot()
				.FillWidth(0.5f)
				.HAlign(HAlign_Right)
				[
					SNew(SUniformGridPanel)
					.SlotPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f))
					+SUniformGridPanel::Slot(0, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(LOCTEXT("TeamCityExecutePersonalBuild", "Run Personal Build"))
						.OnClicked( this, &STeamCityRunPersonalBuildWidget::OnRunPersonalBuild_Clicked )
						.IsEnabled( this, &STeamCityRunPersonalBuildWidget::IsRunPersonalBuildEnabled)
					]
					+SUniformGridPanel::Slot(1, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(LOCTEXT("TeamCitySettingsClose", "Close"))
						.OnClicked( this, &STeamCityRunPersonalBuildWidget::OnClose_Clicked)
					]
				]
			]
		]
	];
#endif
	FTeamCityEditorModule& Module = FTeamCityEditorModule::LoadModuleChecked();
	Module.OnConnectionFlushComplete().AddSP(this, &STeamCityRunPersonalBuildWidget::OnConnectionFlushComplete);

	TeamCityGetBuildConfig = TSharedPtr<FTeamCityGetBuildConfig>(new FTeamCityGetBuildConfig);
	TeamCityGetBuildConfig->OnGetBuildsComplete().AddSP(this, &STeamCityRunPersonalBuildWidget::OnGetBuildConfigs);
	TeamCityGetBuildConfig->Process();
}

void STeamCityRunPersonalBuildWidget::OnConnectionFlushComplete(ETeamCityConnectionState TeamCityConnectionState)
{
	TeamCityGetBuildConfig->Process();
}

FReply STeamCityRunPersonalBuildWidget::OnRunPersonalBuild_Clicked()
{
	if (FileSelectWidget.IsValid())
	{
		// copy across each file and their previous revision to their respective working directories
		if (FTaskGraphInterface::IsRunning())
		{
			IsProcessingPersonalBuild = true;

			const TArray<FFileItem> Files(FileSelectWidget.Pin()->GetSelectedFiles());
			FFunctionGraphTask::CreateAndDispatchWhenReady([Files, this]()
			{
				TArray<uint8> TeamCityPatch(STeamCityRunPersonalBuildWidget::GenerateTeamCityPatch(Files));

				FString URLFriendlyDescription(Description.ToString());
				URLFriendlyDescription.ReplaceInline(TEXT(" "), TEXT("%20")); 
				URLFriendlyDescription.ReplaceInline(TEXT("\r\n"), TEXT("%0A")); 

				// now we have our patch, we can invoke a personal build!				
				const FString TeamCityPostChangesString(FString(TEXT("uploadChanges.html?description=") + URLFriendlyDescription + FString(TEXT("&commitType=0"))));
				
				FTeamCityWebAPI::PerformPostRequest(TeamCityPostChangesString, "text/text", TeamCityPatch, this, &STeamCityRunPersonalBuildWidget::OnPostedChanges);
			}, 
			TStatId(), nullptr, ENamedThreads::AnyThread);
		}

	}
	return FReply::Handled();
}

void STeamCityRunPersonalBuildWidget::OnPostedChanges(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	// once we've finished posting our chages, its time to construct the request to start a new personal build
	// note this process is mostly covered here: https://www.jetbrains.com/help/teamcity/personal-build.html#Direct+Patch+Upload

	IsProcessingPersonalBuild = bSucceeded && HttpResponse->GetResponseCode() == 201;
	if (IsProcessingPersonalBuild)
	{
		// let's get that change ID from the server
		const FString ChangeID(HttpResponse->GetContentAsString());

		TArray<RunPersonalBuildItemPtr> SelectedConfigs(GetSelectedConfigs(Items[0]));
		for (auto& Config : SelectedConfigs)
		{
			// set up each build config's xml definition
			FString BuildConfiguration = TEXT("\
			<build personal = \"true\">\
				<triggered type = 'UE4' details = 'Absolute Patch'/>\
				<triggeringOptions cleanSources = \"false\" rebuildAllDependencies = \"false\" queueAtTop = \"false\"/>\
				<buildType id = \"<build_configuration_id>\"/>\
				<comment><text><description></text></comment>\
				<lastChanges>\
					<change id = \"<change_ID>\" personal = \"true\"/>\
				</lastChanges>\
			</build>\
			");
			
			BuildConfiguration.ReplaceInline(TEXT("<build_configuration_id>"), *Config->GetID());
			BuildConfiguration.ReplaceInline(TEXT("<change_ID>"), *ChangeID);
			BuildConfiguration.ReplaceInline(TEXT("<description>"), *(Description.ToString()));

			const FTCHARToUTF8 ANSIString(*BuildConfiguration);
			const ANSICHAR* ANSIBytes = ANSIString.Get();
			const int32 ANSIByteSize = ANSIString.Length();
			TArray<uint8> Payload;
			Payload.Append((uint8*)ANSIBytes, ANSIByteSize * sizeof(ANSICHAR));

			// and trigger the personal build!
			FString TeamCityPostBuildString(TEXT("app/rest/buildQueue"));
			FTeamCityWebAPI::PerformPostRequest(TeamCityPostBuildString, TEXT("application/xml"), Payload, this, &STeamCityRunPersonalBuildWidget::OnTriggeredPersonalBuild);
		}
	}
	else
	{
		UE_LOG(LogTeamCityEditor, Error, TEXT("Posting changes failed: %s"), *HttpResponse->GetContentAsString());
	}
}

void STeamCityRunPersonalBuildWidget::OnTriggeredPersonalBuild(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	const bool HasTriggeredPersonalBuild = bSucceeded && HttpResponse->GetResponseCode() == 200;
	if (HasTriggeredPersonalBuild)
	{
		FXmlFile XmlFile;
		const bool XmlParseSuccess = FTeamCityWebAPI::ParseTeamCityResponse(XmlFile, HttpResponse);

		if (XmlParseSuccess)
		{
			const FXmlNode* RootNode = XmlFile.GetRootNode();
			
			const FXmlNode* BuildTypeNode = RootNode->FindChildNode(TEXT("buildType"));
			check(BuildTypeNode != nullptr);

			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("ProjectName"), FText::FromString(BuildTypeNode->GetAttribute(TEXT("projectName"))));
			Arguments.Add(TEXT("Name"), FText::FromString(BuildTypeNode->GetAttribute(TEXT("name"))));
			const FText NotificationText = FText::Format(LOCTEXT("TCBuildNotification", "Running {ProjectName} {Name} build..."), Arguments);

			const FString BuildIDhref(TEXT("app/rest/builds/id:") + RootNode->GetAttribute(TEXT("id")));

			FTeamCityBuildNotificationsHelper::AddTeamCityBuildNotification(NotificationText, RootNode->GetAttribute(TEXT("webUrl")), RootNode->GetAttribute(TEXT("id")));
		}
			   
		if (ParentWindowPtr.IsValid())
		{
			ParentWindowPtr.Pin()->RequestDestroyWindow();
		}
	}
	else
	{
		UE_LOG(LogTeamCityEditor, Error, TEXT("Triggering personal build failed: %s"), *HttpResponse->GetContentAsString());
	}

	IsProcessingPersonalBuild = false;
}

EVisibility STeamCityRunPersonalBuildWidget::GetProcessingPersonalBuildThrobberVisibility() const
{
	return IsProcessingPersonalBuild ? EVisibility::Visible : EVisibility::Collapsed;
}

uint16 FormatForBigEndianInt16(uint16 Value)
{
	uint16 ReturnValue = 0;
	if (FPlatformProperties::IsLittleEndian())
	{
		uint8* ReturnValueBytePtr = (uint8*)&ReturnValue;
		*(ReturnValueBytePtr + 0) = Value >> 8;
		*(ReturnValueBytePtr + 1) = Value >> 0;
	}
	else
	{
		ReturnValue = Value;
	}
	return ReturnValue;
}

uint64 FormatForBigEndianInt64(uint64 Value)
{
	uint64 ReturnValue = 0;
	if (FPlatformProperties::IsLittleEndian())
	{
		uint8* ReturnValueBytePtr = (uint8*)&ReturnValue;
		*(ReturnValueBytePtr + 0) = Value >> 56;
		*(ReturnValueBytePtr + 1) = Value >> 48;
		*(ReturnValueBytePtr + 2) = Value >> 40;
		*(ReturnValueBytePtr + 3) = Value >> 32;
		*(ReturnValueBytePtr + 4) = Value >> 24;
		*(ReturnValueBytePtr + 5) = Value >> 16;
		*(ReturnValueBytePtr + 6) = Value >> 8;
		*(ReturnValueBytePtr + 7) = Value >> 0;
	}
	else
	{
		ReturnValue = Value;
	}
	return ReturnValue;
}

void InjectFilePathIntoPatch(TArray<uint8>& PatchData, const FString& FilePath)
{
	// Teamcity expects the path to be prefixed with a 16 bit integer denoting the length of the path
	const uint16 FilePathLength = FilePath.Len();
	const int32 ByteIndexOfPathLength = PatchData.Num();
	PatchData.AddUninitialized(sizeof(uint16));

	uint16* PatchPatchLengthPtr = (uint16*)(PatchData.GetData() + ByteIndexOfPathLength);
	*PatchPatchLengthPtr = FormatForBigEndianInt16(FilePathLength); // Teamcity expects the path length value in big-endian format

	const FTCHARToUTF8 ANSIHeaderString(*FilePath);
	const ANSICHAR* ANSIHeaderBytes = ANSIHeaderString.Get();
	const int32 ANSIHeaderByteSize = ANSIHeaderString.Length();
	PatchData.Append((uint8*)ANSIHeaderBytes, ANSIHeaderByteSize * sizeof(ANSICHAR));
}

TArray<uint8> STeamCityRunPersonalBuildWidget::GenerateTeamCityPatch(const TArray<FFileItem> Files) const
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	TArray<uint8> TeamCityPatch;

	FTeamCityEditorModule& Module = FTeamCityEditorModule::LoadModuleChecked();
	const FString ProjectDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()));

	for (int32 i = 0; i < Files.Num(); i++)
	{
		const FFileItem& CandidateFile = Files[i];
		const FString FilePath(CandidateFile.Path);
		IFileHandle* MyFileHandle = PlatformFile.OpenRead(*FilePath);

		const FString RelativePath(FilePath.RightChop(ProjectDirectory.Len()));
		const FString SourceControlFilePath(TeamCityGetBuildConfig->FormatPathRelativeToSourceControl(RelativePath));

		/* Team City describe patch files as formatted as below, however we are treating all files as binary:			
		1 - byte indicator of command(create binary = 25, replace binary file = 26, create text file = 28, replace text file = 27)
		length of the file
		file_data */

		uint8 HeaderByte = -1;
		switch (CandidateFile.ChangeType)
		{
		case EChangeType::Add:
			HeaderByte = 0x1a;
			break;
		case EChangeType::Remove:
			HeaderByte = 0x03;
			break;
		case EChangeType::Change:
			HeaderByte = 0x19;
			break;
		case EChangeType::None:
		default:
			checkf(0, TEXT("Please add the correct header byte for this change type"));
			break;
		}

		TeamCityPatch.Emplace(HeaderByte);
		InjectFilePathIntoPatch(TeamCityPatch, SourceControlFilePath);

		if (CandidateFile.ChangeType != EChangeType::Remove)
		{
			// write the footer
			TArray<uint8> FooterData;
			FooterData.Emplace(0x0d);

			// Teamcity expects the filesize to be a 64bit integer
			const uint64 FileSize = MyFileHandle->Size();
			const int32 ByteIndexOfFileSize = TeamCityPatch.Num();
			TeamCityPatch.AddUninitialized(sizeof(uint64));

			uint64* PatchFileSizePtr = (uint64*)(TeamCityPatch.GetData() + ByteIndexOfFileSize);
			*PatchFileSizePtr = FormatForBigEndianInt64(MyFileHandle->Size()); // Teamcity expects the filesize value in big-endian format

			// write the file bytes
			TArray<uint8> FileBytes;
			FileBytes.AddZeroed(MyFileHandle->Size());

			if (FileSize > 0)
			{
				MyFileHandle->Read(&FileBytes[0], MyFileHandle->Size());
			}

			TeamCityPatch.Append(FileBytes);

			InjectFilePathIntoPatch(FooterData, SourceControlFilePath);
			FooterData.Append({ 0x00, 0x05, 0x61, 0x2b, 0x72, 0x77, 0x78 });
			TeamCityPatch.Append(FooterData);
		}

		delete MyFileHandle; // close the file
	}

	TeamCityPatch.Emplace(0x0a);
	TeamCityPatch.Emplace(0x00);
	TeamCityPatch.Emplace(0x00);
	
	return TeamCityPatch;
}

FReply STeamCityRunPersonalBuildWidget::OnClose_Clicked()
{
	if (ParentWindowPtr.IsValid())
	{
		ParentWindowPtr.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}

void STeamCityRunPersonalBuildWidget::OnGetChildren(RunPersonalBuildItemPtr InItem, TArray<RunPersonalBuildItemPtr>& OutChildren)
{
	OutChildren = InItem->Children;
}

TSharedRef<ITableRow> STeamCityRunPersonalBuildWidget::OnGenerateRow(RunPersonalBuildItemPtr InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return
		SNew(STableRow<RunPersonalBuildItemPtr>, OwnerTable)
		.Padding(FMargin(1.0f, 2.0f))
		[
			SNew(SBorder)
			.Padding(FMargin(3))
			.BorderImage(InItem->Children.Num() > 0 ? FEditorStyle::GetBrush("ToolPanel.GroupBorder") : FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(0.0f, 1.0f, 1.0f, 1.0f)
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([InItem] { return InItem->GetCheckState(); })
					.OnCheckStateChanged_Lambda([InItem](ECheckBoxState State) { InItem->SetCheckState(State); })
				]
				+ SHorizontalBox::Slot()
				.Padding(0.0f, 1.0f, 0.0f, 1.0f)
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(InItem->GetName()))
				]
			]
		];
}

void STeamCityRunPersonalBuildWidget::SetTreeItemExpansionRecursive(RunPersonalBuildItemPtr TreeItem, bool bInExpansionState) const
{
	BuildConfigTreeView->SetItemExpansion(TreeItem, bInExpansionState);

	for (auto&& Child : TreeItem->Children)
	{
		SetTreeItemExpansionRecursive(Child, bInExpansionState);
	}
}

TArray<RunPersonalBuildItemPtr> STeamCityRunPersonalBuildWidget::GetSelectedConfigs(RunPersonalBuildItemPtr TreeItem) const
{
	TArray<RunPersonalBuildItemPtr> SelectedItems;

	if (TreeItem->IsARunnableBuildConfig() && TreeItem->GetCheckState() == ECheckBoxState::Checked) { SelectedItems.Add(TreeItem); }

	for (auto&& Child : TreeItem->Children)
	{
		SelectedItems.Append(GetSelectedConfigs(Child));
	}

	return SelectedItems;
}

bool STeamCityRunPersonalBuildWidget::IsRunPersonalBuildEnabled() const
{
	const bool HasDescription = Description.IsEmpty() == false;
	
	bool HasSelectedAConfig = false;
	if (Items.Num() > 0) { HasSelectedAConfig = GetSelectedConfigs(Items[0]).Num() > 0; }

	bool HasSelectedFiles = FileSelectWidget.Pin()->GetSelectedFiles().Num() > 0;

	return HasDescription && HasSelectedAConfig && HasSelectedFiles && (IsProcessingPersonalBuild == false);
}

void STeamCityRunPersonalBuildWidget::SetDescription(const FText& InDescription, ETextCommit::Type InCommitType)
{
	Description = InDescription;
}

RunPersonalBuildItemPtr ConvertToRunPersonalBuildItem(BuildConfigPtr BuildConfig)
{
	RunPersonalBuildItemPtr RunPersonalBuildItem = RunPersonalBuildItemPtr(new FRunPersonalBuildItem(BuildConfig));
	for (auto& Child : BuildConfig->GetChildren())
	{
		RunPersonalBuildItem->Children.Emplace(ConvertToRunPersonalBuildItem(Child));
		RunPersonalBuildItem->Children.Last()->Parent = RunPersonalBuildItem;
	}
	return RunPersonalBuildItem;
}

void STeamCityRunPersonalBuildWidget::OnGetBuildConfigs(BuildConfigPtr RootBuildConfig)
{
	Items.Empty();

	if (RootBuildConfig.IsValid())
	{
		// copy builds from RootBuildConfig to our set
		RunPersonalBuildItemPtr RunPersonalBuildRootItem = ConvertToRunPersonalBuildItem(RootBuildConfig);

		SetTreeItemExpansionRecursive(RunPersonalBuildRootItem, true);
		Items.Emplace(RunPersonalBuildRootItem);
	}

	BuildConfigTreeView->RequestTreeRefresh();
}

#undef LOCTEXT_NAMESPACE
