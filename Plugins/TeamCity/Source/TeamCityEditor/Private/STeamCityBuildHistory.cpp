#include "STeamCityBuildHistory.h"
#include "TeamCityEditorPCH.h"
#include "TeamCityWebAPI.h"
#include "Widgets/Input/SCheckBox.h"
#include "Framework/Commands/UIAction.h"
#include "Textures/SlateIcon.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ScopedTransaction.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Widgets/Input/SHyperlink.h"
#include "TeamCityStyle.h"

#include "TeamCityEditor.h"

#define LOCTEXT_NAMESPACE "STeamCityBuildsViewer"

static const FName ColumnID_BuildTimeStampLabel("Build Finished");
static const FName ColumnID_BuildType("Build Type");
static const FName ColumnID_BuildStatus("Status");
static const FName ColumnID_Description("Comment");

//////////////////////////////////////////////////////////////////////////
// STeamCityBuildListRow

typedef TSharedPtr<FDisplayedTeamCityBuildInfo> FDisplayedTeamCityBuildInfoPtr;

class STeamCityBuildListRow
	: public SMultiColumnTableRow<FDisplayedTeamCityBuildInfoPtr>
{
public:

	SLATE_BEGIN_ARGS(STeamCityBuildListRow) {}

	/** The item for this row **/
	SLATE_ARGUMENT(FDisplayedTeamCityBuildInfoPtr, Item)

	/* The STeamCityBuildsViewer that we push the Team City builds into */
	SLATE_ARGUMENT(class STeamCityBuildHistory*, TeamCityBuildsViewer)

	/* Widget used to display the list of Team City builds */
	SLATE_ARGUMENT(TSharedPtr<STeamCityBuildListType>, TeamCityBuildsListView)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView);

	/** Overridden from SMultiColumnTableRow.  Generates a widget for this column of the tree row. */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:

	/* Execute the delegate for the hyperlink, if bound */
	void OnHyperlinkClicked(FSimpleDelegate Hyperlink) const
	{
		Hyperlink.ExecuteIfBound();
	}

	/* The STeamCityBuildsViewer that we push the Team City builds into */
	STeamCityBuildHistory* TeamCityBuildsViewer;

	/** Widget used to display the list of Team City builds */
	TSharedPtr<STeamCityBuildListType> TeamCityBuildsListView;

	/** The Team City build info */
	FDisplayedTeamCityBuildInfoPtr	Item;
};

void STeamCityBuildListRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	Item = InArgs._Item;
	TeamCityBuildsViewer = InArgs._TeamCityBuildsViewer;
	TeamCityBuildsListView = InArgs._TeamCityBuildsListView;

	check(Item.IsValid());

	SMultiColumnTableRow<FDisplayedTeamCityBuildInfoPtr>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
}

TSharedRef<SWidget> STeamCityBuildListRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (ColumnName == ColumnID_BuildTimeStampLabel)
	{		
		return
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SHyperlink)
					.Text(FText::FromString(Item->DateTimeFinished))
					.OnNavigate(this, &STeamCityBuildListRow::OnHyperlinkClicked, Item->Hyperlink)
				]
			];
	}
	else if (ColumnName == ColumnID_BuildType)
	{
		return
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SHyperlink)
					.Text(FText::FromString(Item->ProjectName + FString(TEXT(" / ")) + Item->Name))
					.OnNavigate(this, &STeamCityBuildListRow::OnHyperlinkClicked, Item->Hyperlink)
				]
			];
	}
	else if (ColumnName == ColumnID_BuildStatus)
	{
		return
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(0.0f, 1.0f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SImage)
				.Image(Item->BuildStatusBrush)
			];
	}
	else if (ColumnName == ColumnID_Description)
	{
		return
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Item->Comment))
				]
			];
	}
	else
	{
		return
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f, 4.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("NoDataFound", "NO DATA"))
				]
			];
	}
}

//////////////////////////////////////////////////////////////////////////
// STeamCityBuildsViewer

void STeamCityBuildHistory::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.Padding(FMargin(3))
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight( 1.0f )		// This is required to make the scrollbar work, as content overflows Slate containers by default
			[
				SAssignNew(TeamCityBuildsListView, STeamCityBuildListType)
				.ListItemsSource(&TeamCityBuildsList)
				.OnGenerateRow(this, &STeamCityBuildHistory::GenerateTeamCityBuildRow)
				.ItemHeight(22.0f)
				.SelectionMode(ESelectionMode::None)
				.HeaderRow
				(
					SNew(SHeaderRow)
					+ SHeaderRow::Column(ColumnID_BuildStatus)
					.FixedWidth(20.0f)
					.DefaultLabel(LOCTEXT("BuildStatusLabel", ""))

					+ SHeaderRow::Column(ColumnID_BuildTimeStampLabel)
					.ManualWidth(128.0f)
					.DefaultLabel(LOCTEXT("BuildTimeStartedLabel", "Date/Time"))

					+ SHeaderRow::Column(ColumnID_BuildType)
					.ManualWidth(320.0f)
					.DefaultLabel(LOCTEXT("BuildTypeLabel", "Type"))

					+ SHeaderRow::Column(ColumnID_Description)
					.DefaultLabel(LOCTEXT("DescriptionLabel", "Description"))
				)
			]
		]
	];

	FTeamCityEditorModule& Module = FTeamCityEditorModule::LoadModuleChecked();
	Module.OnConnectionFlushComplete().AddSP(this, &STeamCityBuildHistory::OnConnectionFlushComplete);

	TeamCityGetBuildConfig = TSharedPtr<FTeamCityGetBuildConfig>(new FTeamCityGetBuildConfig);
	TeamCityGetBuildConfig->OnGetBuildsComplete().AddSP(this, &STeamCityBuildHistory::OnGetBuildConfigs);
	TeamCityGetBuildConfig->Process();
	TimeSinceLastProcess = 0.0f;
}

void STeamCityBuildHistory::OnConnectionFlushComplete(ETeamCityConnectionState TeamCityConnectionState)
{
	TeamCityGetBuildConfig->Process();
}

void STeamCityBuildHistory::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	static const float ProcessPeriod = 30.0f;
	TimeSinceLastProcess += InDeltaTime;
	if (TimeSinceLastProcess >= ProcessPeriod)
	{
		TeamCityGetBuildConfig->Process();
		TimeSinceLastProcess = 0.0f;
	}
}

TSharedRef<ITableRow> STeamCityBuildHistory::GenerateTeamCityBuildRow(TSharedPtr<FDisplayedTeamCityBuildInfo> InInfo, const TSharedRef<STableViewBase>& OwnerTable)
{
	check( InInfo.IsValid() );

	return
		SNew(STeamCityBuildListRow, OwnerTable)
		.Item(InInfo)
		.TeamCityBuildsViewer(this)
		.TeamCityBuildsListView(TeamCityBuildsListView);
}

FString FormatTeamCityDateTime(FString InDateTime)
{	
	{
		InDateTime.InsertAt(4, "-");
		InDateTime.InsertAt(7, "-");
		InDateTime.InsertAt(13, ":");
		InDateTime.InsertAt(16, ":");
		InDateTime.InsertAt(22, ":");
	}

	FDateTime OutDateTime;
	FDateTime::ParseIso8601(*InDateTime, OutDateTime);
	
	return OutDateTime.ToString();
}

void STeamCityBuildHistory::OnGetPersonalBuilds(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	TeamCityBuildsList.Empty();

	const bool GotValidResponse = bSucceeded && HttpResponse->GetResponseCode() == 200;
	if (GotValidResponse)
	{
		FXmlFile XmlFile;
		const bool XmlParseSuccess = FTeamCityWebAPI::ParseTeamCityResponse(XmlFile, HttpResponse);
		if (XmlParseSuccess)
		{
			const FXmlNode* RootNode = XmlFile.GetRootNode();
			const TArray<FXmlNode*>& BuildTypeNodes = RootNode->GetChildrenNodes();

			for (int32 i = 0; i < BuildTypeNodes.Num(); i++)
			{
				const FXmlNode* BuildTypeNode = BuildTypeNodes[i];

				const FSlateBrush* BuildStatusBrush = nullptr;
				{
					const FString BuildStatus(BuildTypeNode->GetAttribute(TEXT("status")));
					if (BuildStatus == "SUCCESS") { BuildStatusBrush = FTeamCityStyle::Get()->GetBrush("TeamCity.Icon.BuildSuccessful"); }
					else if (BuildStatus == "FAILURE") { BuildStatusBrush = FTeamCityStyle::Get()->GetBrush("TeamCity.Icon.BuildFailed"); }
					// @RossB: TODO: Perform a more robust check to determine build cancelled status		
					else { BuildStatusBrush = FTeamCityStyle::Get()->GetBrush("TeamCity.Icon.BuildCanceled"); }
				}
				
				const FString WebURL(BuildTypeNode->GetAttribute(TEXT("webUrl")));

				FString DateTimeFinished, ProjectName, Name, Comment;
				for (auto& BuildTypeChildNode : BuildTypeNode->GetChildrenNodes())
				{
					if (BuildTypeChildNode->GetTag() == TEXT("buildType"))
					{
						ProjectName = FString(BuildTypeChildNode->GetAttribute("projectName"));
						Name = FString(BuildTypeChildNode->GetAttribute("name"));
					}

					if (BuildTypeChildNode->GetTag() == TEXT("finishDate"))
					{
						DateTimeFinished = FString(FormatTeamCityDateTime(BuildTypeChildNode->GetContent()));
					}

					if (BuildTypeChildNode->GetTag() == TEXT("comment"))
					{
						for (auto& CommentChildNode : BuildTypeChildNode->GetChildrenNodes())
						{
							if (CommentChildNode->GetTag() == TEXT("text")) { Comment = CommentChildNode->GetContent(); }
						}
					}
				}

				TSharedRef<FDisplayedTeamCityBuildInfo> NewBuildInfo = FDisplayedTeamCityBuildInfo::Make(DateTimeFinished, ProjectName, Name, Comment, BuildStatusBrush);
				NewBuildInfo->Hyperlink = FSimpleDelegate::CreateStatic(&FTeamCityEditorModule::OpenHyperlink, WebURL);
				TeamCityBuildsList.Add(NewBuildInfo);
			}
		}
	}

	TeamCityBuildsList.Sort([](const TSharedPtr<FDisplayedTeamCityBuildInfo>& A, const TSharedPtr<FDisplayedTeamCityBuildInfo>& B)
	{
		FDateTime ADateTime, BDateTime;
		FDateTime::Parse(A->DateTimeFinished, ADateTime);
		FDateTime::Parse(B->DateTimeFinished, BDateTime);
		return ADateTime > BDateTime;
	});

	TeamCityBuildsListView->RequestListRefresh();
}

void STeamCityBuildHistory::OnGetBuildConfigs(BuildConfigPtr RootBuildConfig)
{
	if(RootBuildConfig.IsValid())
	{
		// we iterate over all the direct children of the root project, as if we queried for at the root project level we would
		// get back *all* personal builds, not just the ones relevant to the current project
		for (auto& ChildBuildConfig : RootBuildConfig->GetChildren())
		{
			UTeamCitySettings* Settings = GetMutableDefault<UTeamCitySettings>();
			FTeamCityWebAPI::PerformGetRequestSP(
				TEXT("app/rest/builds/?locator=affectedProject:" + ChildBuildConfig->GetID() + ",personal:true,user:") + Settings->Username.ToString() + "&fields=build(buildType,status,state,webUrl,startDate,finishDate,comment)",
				this,
				&STeamCityBuildHistory::OnGetPersonalBuilds);
		}
	}
	else
	{
		TeamCityBuildsList.Empty();
		TeamCityBuildsListView->RequestListRefresh();
	}
}

STeamCityBuildHistory::~STeamCityBuildHistory()
{
}

#undef LOCTEXT_NAMESPACE