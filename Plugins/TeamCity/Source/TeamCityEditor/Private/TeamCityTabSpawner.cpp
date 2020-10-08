#include "TeamCityTabSpawner.h"
#include "TeamCityEditorPCH.h"
#include "TeamCityStyle.h"
#include "TeamCityWidget.h"
#include "STeamCityBuildHistory.h"

#if WITH_EDITOR

#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "GetTeamCityMenuCategory.h"

#define LOCTEXT_NAMESPACE "TeamCity"

FTeamCityTabSpawner::FTeamCityTabSpawner()
	: TabSpawnerEntry(nullptr)
	, PersonalBuildWidget(SNew(STeamCityWidget))
	, BuildHistoryWidget(SNew(STeamCityBuildHistory))
{
}

FTeamCityTabSpawner::~FTeamCityTabSpawner()
{}
  
void FTeamCityTabSpawner::Create()
{
#if WITH_EDITOR
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorTabManagerChangedHandle = LevelEditorModule.OnTabManagerChanged().AddLambda([this]()
	{
		FLevelEditorModule& LocalLevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));

		FSlateIcon TeamCityIcon(FTeamCityStyle::Get()->GetStyleSetName(), "TeamCity.Icon.Small");

		static const FName TeamCityPersonalBuildViewportID("TeamCityPersonalBuild");
		LocalLevelEditorModule.GetLevelEditorTabManager()->RegisterTabSpawner(
			TeamCityPersonalBuildViewportID, FOnSpawnTab::CreateSP(this, &FTeamCityTabSpawner::SpawnPersonalBuildTab))
			.SetGroup(GetTeamCityMenuCategory(TeamCityIcon))
			.SetDisplayName(LOCTEXT("PersonalBuild", "Personal Build"))
			.SetTooltipText(LOCTEXT("PersonalBuildTooltipText", "Opens the Personal Build tab."))
			.SetIcon(TeamCityIcon);

		static const FName TeamCityBuildHistoryViewportID("TeamCityBuildHistory");
		LocalLevelEditorModule.GetLevelEditorTabManager()->RegisterTabSpawner(
			TeamCityBuildHistoryViewportID, FOnSpawnTab::CreateSP(this, &FTeamCityTabSpawner::SpawnBuildHistoryTab))
			.SetGroup(GetTeamCityMenuCategory(TeamCityIcon))
			.SetDisplayName(LOCTEXT("BuildHistory", "Build History"))
			.SetTooltipText(LOCTEXT("BuildHistoryTooltipText", "Opens the personal build history tab."))
			.SetIcon(TeamCityIcon);
	});
#endif
}

void FTeamCityTabSpawner::Destroy()
{
#if WITH_EDITOR
	if (FModuleManager::Get().IsModuleLoaded(TEXT("LevelEditor")))
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
		LevelEditorModule.OnTabManagerChanged().Remove(LevelEditorTabManagerChangedHandle);
	}
#endif
}

TSharedRef<SDockTab> FTeamCityTabSpawner::SpawnPersonalBuildTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab);
	SpawnedTab->SetContent(PersonalBuildWidget);
	return SpawnedTab;
}

TSharedRef<SDockTab> FTeamCityTabSpawner::SpawnBuildHistoryTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab);
	SpawnedTab->SetContent(BuildHistoryWidget);
	return SpawnedTab;
}

#undef LOCTEXT_NAMESPACE

#endif
