#include "TeamCityConnectionStatusWidget.h"
#include "TeamCityStyle.h"
#include "TeamCitySettingsWidget.h"
#include "TeamCityEditorPCH.h"

#if WITH_EDITOR
#include "EditorStyleSet.h"
#endif

#define LOCTEXT_NAMESPACE "TeamCity"

void STeamCityConnectionStatusWidget::Construct(const FArguments& InArgs)
{
#if WITH_EDITOR
	const FTeamCityEditorModule& Module = FTeamCityEditorModule::LoadModuleChecked();

	ChildSlot
	[
		SNew(SButton)
		.ButtonStyle(FEditorStyle::Get(), "FlatButton")
		.ToolTipText(LOCTEXT("TeamCityConnectionSettingToolTip", "Change connection settings"))
		.OnClicked(this, &STeamCityConnectionStatusWidget::OnClicked)
		[
			SNew(SImage)
			.Image(FTeamCityStyle::Get()->GetBrush("TeamCity.Icon.Connection"))
			.ColorAndOpacity(this, &STeamCityConnectionStatusWidget::GetConnectionColor)
		]
	];
#endif
}

FSlateColor STeamCityConnectionStatusWidget::GetConnectionColor() const
{
	FSlateColor ConnectionColor;

	const FTeamCityEditorModule& Module = FTeamCityEditorModule::LoadModuleChecked();
	switch (Module.GetConnectionState())
	{
	default:
	case ETeamCityConnectionState::Disconnected: { ConnectionColor = FLinearColor::Red; break; }
	case ETeamCityConnectionState::Connecting: { ConnectionColor = FLinearColor(FColor::Orange); break; }
	case ETeamCityConnectionState::Connected: { ConnectionColor = FLinearColor::Green; break; }
	}

	return ConnectionColor;
}

FReply STeamCityConnectionStatusWidget::OnClicked()
{
	TSharedPtr<SWindow> SettingsWindowPtr = SNew(SWindow)
		.Title(LOCTEXT("TeamCitySettingsWindowTitle", "TeamCity Settings"))
		.HasCloseButton(false)
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.SizingRule(ESizingRule::Autosized);

	SettingsWindowPtr->SetContent(
		SNew(SBox)
		.WidthOverride(300.0f)
		.HeightOverride(100.0f)
		[
			SNew(STeamCitySettingsWidget)
			.ParentWindow(SettingsWindowPtr)
		]
	);

	TSharedPtr<SWindow> RootWindow = FGlobalTabmanager::Get()->GetRootWindow();
	FSlateApplication::Get().AddWindowAsNativeChild(SettingsWindowPtr.ToSharedRef(), RootWindow.ToSharedRef());

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
