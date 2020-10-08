#pragma once
#include "CoreMinimal.h"

#if WITH_EDITOR
#include "Styling/SlateStyle.h"

class FTeamCityStyleSet : public FSlateStyleSet
{
public:
	FTeamCityStyleSet();
	virtual ~FTeamCityStyleSet();

private:
	// registers icons for all classes defined by the plugin
	void RegisterIcon(FName PropertyName, const FString& IconFileName, const ANSICHAR* Extension, const FVector2D& Dimensions);
};


class FTeamCityStyle
{
public:
	static void Create();
	static void Destroy();
	static TSharedPtr<class ISlateStyle> Get();

private:
	static TSharedPtr<class FTeamCityStyleSet> StyleSet;
};
#endif
