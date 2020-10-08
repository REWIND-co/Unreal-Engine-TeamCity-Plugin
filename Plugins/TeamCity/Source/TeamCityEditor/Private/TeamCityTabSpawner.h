#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "UObject/GCObject.h"

class FTeamCityTabSpawner : public TSharedFromThis<FTeamCityTabSpawner>
{
public:
	FTeamCityTabSpawner();
	~FTeamCityTabSpawner();

	void Create();
	void Destroy();

	TSharedRef<class SDockTab> SpawnPersonalBuildTab(const class FSpawnTabArgs& Args);
	TSharedRef<class SDockTab> SpawnBuildHistoryTab(const FSpawnTabArgs& Args);

private:
	struct FTabSpawnerEntry* TabSpawnerEntry;
	FDelegateHandle LevelEditorTabManagerChangedHandle;

	TSharedRef<class STeamCityWidget> PersonalBuildWidget;
	TSharedRef<class STeamCityBuildHistory> BuildHistoryWidget;
};

#endif
