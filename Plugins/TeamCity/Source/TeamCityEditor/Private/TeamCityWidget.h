#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "TeamCityFileSelectWidget.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Misc/Optional.h"

class STeamCityWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STeamCityWidget)
	{}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply RunPersonalBuild_OnClicked();
	FReply Refresh_OnClicked();

	bool IsConnected() const;
	bool CanRunPersonalBuild() const;
	bool CanRefresh() const;
	EVisibility GetRefreshingFilesThrobberVisibility() const;
	
	TSharedPtr<SButton> RefreshButtonWidget;
	TSharedPtr<STeamCityFileSelectWidget> FileSelectWidget;
};
