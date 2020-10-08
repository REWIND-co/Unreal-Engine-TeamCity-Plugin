#include "TeamCityStyle.h"
#include "TeamCityEditorPCH.h"

#if WITH_EDITOR

#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/FileManager.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UObjectBaseUtility.h"

static FString InContent(const FString& RelativePath, const ANSICHAR* Extension)
{
	auto myself = IPluginManager::Get().FindPlugin(TEXT("TeamCity"));
	check(myself.IsValid());
	static FString ContentDir = myself->GetBaseDir() / TEXT("Resources");
	return (ContentDir / RelativePath) + Extension;
}

TSharedPtr<FTeamCityStyleSet> FTeamCityStyle::StyleSet = NULL;
TSharedPtr<class ISlateStyle> FTeamCityStyle::Get() { return StyleSet; }

static FString GetPluginDirectory()
{
	const FString ProjectResourceDir = FPaths::ProjectPluginsDir() / TEXT("TeamCity");
	const FString EngineResourceDir = FPaths::EnginePluginsDir() / TEXT("TeamCity");
	//Is the plugin in the project? In that case, use those resources
	return IFileManager::Get().DirectoryExists(*ProjectResourceDir) ? ProjectResourceDir : EngineResourceDir;
}

FTeamCityStyleSet::FTeamCityStyleSet()
	: FSlateStyleSet("TeamCityStyle")
{
	const FString ResourceDir = GetPluginDirectory() / TEXT("/Resources");
	SetContentRoot(ResourceDir);
	SetCoreContentRoot(ResourceDir);

	const FVector2D Icon16x16(16.0f, 16.0f);
	const FVector2D Icon20x20(20.0f, 20.0f);
	const FVector2D Icon40x40(40.0f, 40.0f);

	RegisterIcon("TeamCity.Icon.Small", "Icon16", ".png", Icon16x16);
	RegisterIcon("TeamCity.Icon.Medium", "Icon40", ".png", Icon40x40);

	RegisterIcon("TeamCity.Icon.RunPersonalBuild", "RunPersonalBuild", ".png", Icon16x16);
	RegisterIcon("TeamCity.Icon.Refresh", "Refresh", ".png", Icon16x16);  
	RegisterIcon("TeamCity.Icon.Connection", "Connection", ".png", Icon16x16);
	RegisterIcon("TeamCity.Icon.BuildSuccessful", "BuildSuccessful", ".png", Icon16x16);
	RegisterIcon("TeamCity.Icon.BuildCanceled", "BuildCanceled", ".png", Icon16x16);
	RegisterIcon("TeamCity.Icon.BuildFailed", "BuildFailed", ".png", Icon16x16);
	RegisterIcon("TeamCity.Icon.BuildNotification", "Icon40", ".png", Icon40x40);

	FSlateStyleRegistry::RegisterSlateStyle(*this);
}

FTeamCityStyleSet::~FTeamCityStyleSet()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*this);
}

void FTeamCityStyleSet::RegisterIcon(FName PropertyName, const FString& IconFileName, const ANSICHAR* Extension, const FVector2D& Dimensions)
{
	FSlateImageBrush* IconBrush = new FSlateImageBrush(InContent(IconFileName, Extension), Dimensions);
	Set(PropertyName, IconBrush);
}

void FTeamCityStyle::Create()
{
	if (StyleSet.IsValid() == false)
	{
		StyleSet = MakeShareable(new FTeamCityStyleSet());
	}
};

void FTeamCityStyle::Destroy()
{
	if (StyleSet.IsValid())
	{
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}

#endif
