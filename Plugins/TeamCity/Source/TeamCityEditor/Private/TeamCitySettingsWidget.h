#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWindow.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Misc/Optional.h"

class STeamCitySettingsWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STeamCitySettingsWidget) {}
		SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply OnConnect_Clicked();
	FReply OnClose_Clicked();

	EVisibility GetConnectionThrobberVisibility() const;

	FText GetServer() const;
	FText GetUsername() const;
	FText GetPassword() const;

	void SetServer(const FText& InServer, ETextCommit::Type InCommitType);
	void SetUsername(const FText& InUsername, ETextCommit::Type InCommitType);
	void SetPassword(const FText& InPassword, ETextCommit::Type InCommitType);

	TWeakPtr<SWindow> ParentWindowPtr;
};
