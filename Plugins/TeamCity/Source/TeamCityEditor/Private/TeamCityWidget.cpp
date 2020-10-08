#include "TeamCityWidget.h"
#include "TeamCityStyle.h"
#include "TeamCityFileSelectWidget.h"
#include "TeamCityRunPersonalBuildWidget.h"
#include "TeamCityEditorPCH.h"
#include "TeamCityBuildNotificationsHelper.h"
#include "TeamCityConnectionStatusWidget.h"

#if WITH_EDITOR
#include "EditorStyleSet.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SThrobber.h"
#endif

#define LOCTEXT_NAMESPACE "TeamCity"

void STeamCityWidget::Construct(const FArguments& InArgs)
{
#if WITH_EDITOR
	const FTeamCityEditorModule& Module = FTeamCityEditorModule::LoadModuleChecked();

	ChildSlot
	[
		SNew(SVerticalBox) 
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(0.0f, 3.0f, 6.0f, 3.0f)
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FEditorStyle::Get(), "FlatButton")
					.ToolTipText(LOCTEXT("TeamCityRunPersonalBuild", "Run a personal build with the selected changes"))
					.OnClicked(this, &STeamCityWidget::RunPersonalBuild_OnClicked)
					.IsEnabled(this, &STeamCityWidget::CanRunPersonalBuild)
					[
						SNew(SImage)
						.Image(FTeamCityStyle::Get()->GetBrush("TeamCity.Icon.RunPersonalBuild"))
					]
				]
				+ SHorizontalBox::Slot()
				.Padding(0.0f, 3.0f, 6.0f, 3.0f)
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SAssignNew(RefreshButtonWidget, SButton)
					.ButtonStyle(FEditorStyle::Get(), "FlatButton")
					.ToolTipText(LOCTEXT("TeamCityRefresh", "Refresh source control view"))
					.OnClicked(this, &STeamCityWidget::Refresh_OnClicked)
					.IsEnabled(this, &STeamCityWidget::CanRefresh)
					[
						SNew(SImage)
						.Image(FTeamCityStyle::Get()->GetBrush("TeamCity.Icon.Refresh"))
					]
				]
			]
			+ SHorizontalBox::Slot()
			.Padding(0.0f, 3.0f, 6.0f, 3.0f)
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right)
			[
				SNew(SThrobber)
				.Visibility(this, &STeamCityWidget::GetRefreshingFilesThrobberVisibility)
			]
			+ SHorizontalBox::Slot()
			.Padding(0.0f, 3.0f, 6.0f, 3.0f)
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right)
			[
				SNew(STeamCityConnectionStatusWidget)
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(FileSelectWidget, STeamCityFileSelectWidget)
		]
	];
#endif
}

FReply STeamCityWidget::RunPersonalBuild_OnClicked()
{
	bool AlreadyShowingWindow = false;
	const FText RunPersonalBuildWindowTitle(LOCTEXT("TeamCityRunPersonalBuildWindowTitle", "Run Personal Build"));

	TSharedPtr<SWindow> RootWindow = FGlobalTabmanager::Get()->GetRootWindow();
	for (auto& ChildWindow : RootWindow->GetChildWindows())
	{
		AlreadyShowingWindow |= ChildWindow->GetTitle().EqualTo(RunPersonalBuildWindowTitle);
	}

	if(AlreadyShowingWindow == false)
	{
		TSharedPtr<SWindow> SettingsWindowPtr = SNew(SWindow)
			.Title(RunPersonalBuildWindowTitle)
			.HasCloseButton(false)
			.SupportsMaximize(false)
			.SupportsMinimize(false)
			.SizingRule(ESizingRule::Autosized);

		SettingsWindowPtr->SetContent(
			SNew(SBox)
			.WidthOverride(600.0f)
			.HeightOverride(500.0f)
			[
				SNew(STeamCityRunPersonalBuildWidget)
				.ParentWindow(SettingsWindowPtr)
			.FileSelectWidget(FileSelectWidget)
			]
		);

		FSlateApplication::Get().AddWindowAsNativeChild(SettingsWindowPtr.ToSharedRef(), RootWindow.ToSharedRef());
	}

	return FReply::Handled();
}

FReply STeamCityWidget::Refresh_OnClicked()
{
	FileSelectWidget->RefreshItems();
	return FReply::Handled();
}

bool STeamCityWidget::IsConnected() const
{
	const FTeamCityEditorModule& Module = FTeamCityEditorModule::LoadModuleChecked();
	return Module.GetConnectionState() == ETeamCityConnectionState::Connected;
}

bool STeamCityWidget::CanRunPersonalBuild() const
{
	return 
		IsConnected() &&
		(FileSelectWidget->GetSelectedFiles().Num() > 0);
}

bool STeamCityWidget::CanRefresh() const
{
	return IsConnected() && FileSelectWidget->CanRefresh();
}

EVisibility STeamCityWidget::GetRefreshingFilesThrobberVisibility() const
{
	return FileSelectWidget->IsRefreshingFiles() ? EVisibility::Visible : EVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE
