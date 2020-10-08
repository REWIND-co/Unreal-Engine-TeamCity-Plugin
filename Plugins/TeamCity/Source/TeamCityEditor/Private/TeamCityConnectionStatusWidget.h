#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Misc/Optional.h"

class STeamCityConnectionStatusWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STeamCityConnectionStatusWidget)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply OnClicked();
	FSlateColor GetConnectionColor() const;
};
