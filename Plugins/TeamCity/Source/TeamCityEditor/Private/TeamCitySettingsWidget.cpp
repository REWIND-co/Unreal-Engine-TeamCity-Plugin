#include "TeamCitySettingsWidget.h"
#include "TeamCityStyle.h"
#include "TeamCitySettings.h"
#include "TeamCityEditorPCH.h"
#include "TeamCityConnectionStatusWidget.h"

#if WITH_EDITOR
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Images/SThrobber.h"
#endif

#define LOCTEXT_NAMESPACE "TeamCity"

void STeamCitySettingsWidget::Construct(const FArguments& InArgs)
{
	ParentWindowPtr = InArgs._ParentWindow;

	FSlateFontInfo Font = FEditorStyle::GetFontStyle(TEXT("SourceControl.LoginWindow.Font"));
#if WITH_EDITOR
	ChildSlot
	[
		SNew(SBorder)
		.HAlign(HAlign_Fill)
		.BorderImage( FEditorStyle::GetBrush("ChildWindow.Background") )
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 6.0f, 0.0f, 0.0f)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(4.0f)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot()
						.FillHeight(1.0f)
						.Padding(2.0f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("TeamCityServerLabel", "Server"))
							.ToolTipText( LOCTEXT("PortLabel_Tooltip", "The server and port for your TeamCity server. Usage ServerName:1234.") )
							.Font(Font)
						]
						+SVerticalBox::Slot()
						.FillHeight(1.0f)
						.Padding(2.0f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("TeamCityUsernameLabel", "Username"))
							.ToolTipText( LOCTEXT("UserNameLabel_Tooltip", "TeamCity username.") )
							.Font(Font)
						]
						+SVerticalBox::Slot()
						.FillHeight(1.0f)
						.Padding(2.0f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("TeamCityPasswordLabel", "Password"))
							.ToolTipText( LOCTEXT("WorkspaceLabel_Tooltip", "TeamCity password.") )
							.Font(Font)
						]
					]
					+SHorizontalBox::Slot()
					.FillWidth(2.0f)
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot()
						.FillHeight(1.0f)
						.Padding(2.0f)
						[
							SNew(SEditableTextBox)
							.Text(this, &STeamCitySettingsWidget::GetServer)
							.ToolTipText( LOCTEXT("TeamCityServerLabel_Tooltip", "The server and port for your TeamCity server. Usage ServerName:1234.") )
							.Font(Font)
							.OnTextCommitted(this, &STeamCitySettingsWidget::SetServer)
							.OnTextChanged(this, &STeamCitySettingsWidget::SetServer, ETextCommit::Default)
						]
						+SVerticalBox::Slot()
						.FillHeight(1.0f)
						.Padding(2.0f)
						[
							SNew(SEditableTextBox)
							.Text(this, &STeamCitySettingsWidget::GetUsername)
							.ToolTipText( LOCTEXT("TeamCityUsernameLabel_Tooltip", "TeamCity username.") )
							.Font(Font)
							.OnTextChanged(this, &STeamCitySettingsWidget::SetUsername, ETextCommit::Default)
						]
						+SVerticalBox::Slot()
						.FillHeight(1.0f)
						.Padding(2.0f)
						[
							SNew(SEditableTextBox)
							.Text(this, &STeamCitySettingsWidget::GetPassword)
							.ToolTipText( LOCTEXT("TeamCityPasswrdLabel_Tooltip", "TeamCity password.") )
							.Font(Font)
							.OnTextChanged(this, &STeamCitySettingsWidget::SetPassword, ETextCommit::Default)
							.IsPassword(true)
						]
					]
				]
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(8.0f, 16.0f, 0.0f, 8.0f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				[
					SNew(SThrobber)
					.Visibility(this, &STeamCitySettingsWidget::GetConnectionThrobberVisibility)
				]
				+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.HAlign(HAlign_Right)
				[
					SNew(SUniformGridPanel)
					.SlotPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f))
					+SUniformGridPanel::Slot(0, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(LOCTEXT("TeamCitySettingsConnect", "Connect"))
						.OnClicked( this, &STeamCitySettingsWidget::OnConnect_Clicked )
					]
					+SUniformGridPanel::Slot(1, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(LOCTEXT("TeamCitySettingsClose", "Close"))
						.OnClicked( this, &STeamCitySettingsWidget::OnClose_Clicked)
					]
				]
			]
		]
	];
#endif
}

FReply STeamCitySettingsWidget::OnConnect_Clicked()
{
	FTeamCityEditorModule& Module = FTeamCityEditorModule::LoadModuleChecked();
	Module.FlushConnection(); // we flush the connection to clear the current authentication cookie
	return FReply::Handled();
}

FReply STeamCitySettingsWidget::OnClose_Clicked()
{
	if (ParentWindowPtr.IsValid())
	{
		ParentWindowPtr.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}

EVisibility STeamCitySettingsWidget::GetConnectionThrobberVisibility() const
{
	const FTeamCityEditorModule& Module = FTeamCityEditorModule::LoadModuleChecked();
	return Module.GetConnectionState() == ETeamCityConnectionState::Connecting ? EVisibility::Visible : EVisibility::Collapsed;
}

FText STeamCitySettingsWidget::GetServer() const
{
	UTeamCitySettings* Settings = GetMutableDefault<UTeamCitySettings>();
	return Settings->Server;
}

FText STeamCitySettingsWidget::GetUsername() const
{
	UTeamCitySettings* Settings = GetMutableDefault<UTeamCitySettings>();
	return Settings->Username;
}

FText STeamCitySettingsWidget::GetPassword() const
{
	UTeamCitySettings* Settings = GetMutableDefault<UTeamCitySettings>();
	return Settings->Password;
}

void STeamCitySettingsWidget::SetServer(const FText& InServer, ETextCommit::Type InCommitType)
{
	UTeamCitySettings* Settings = GetMutableDefault<UTeamCitySettings>();
	Settings->Server = InServer;
	Settings->PostEditChange();
	Settings->SaveConfig();
}

void STeamCitySettingsWidget::SetUsername(const FText& InUsername, ETextCommit::Type InCommitType)
{
	UTeamCitySettings* Settings = GetMutableDefault<UTeamCitySettings>();
	Settings->Username = InUsername;
	Settings->PostEditChange();
	Settings->SaveConfig();
}

void STeamCitySettingsWidget::SetPassword(const FText& InPassword, ETextCommit::Type InCommitType)
{
	UTeamCitySettings* Settings = GetMutableDefault<UTeamCitySettings>();
	Settings->Password = InPassword;
	Settings->PostEditChange();
	Settings->SaveConfig();
}

#undef LOCTEXT_NAMESPACE
