#pragma once
#include "CoreMinimal.h"
#include "Framework/Docking/WorkspaceItem.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#if WITH_EDITOR
static TSharedRef<FWorkspaceItem> GetTeamCityMenuCategory(const FSlateIcon& IconToUseIfMissing)
{
	TSharedRef<FWorkspaceItem> LevelEditorCategory = WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory();

	int32 TeamCityCategoryIndex = INDEX_NONE;
	const TArray<TSharedRef<FWorkspaceItem>>& RootMenuChildren = LevelEditorCategory->GetChildItems();
	for (int32 i = 0; i < RootMenuChildren.Num(); i++)
	{
		if (RootMenuChildren[i]->GetDisplayName().EqualTo(FText::FromString(TEXT("TeamCity"))))
		{
			TeamCityCategoryIndex = i;
			break;
		}
	}

	if (TeamCityCategoryIndex == INDEX_NONE)
	{
		TSharedRef<FWorkspaceItem> TeamCityCategoryRef = LevelEditorCategory->AddGroup(
			FText::FromString(TEXT("TeamCity")),
			FText::FromString(TEXT("TeamCity tools")),
			IconToUseIfMissing,
			false);
		TeamCityCategoryIndex = RootMenuChildren.Find(TeamCityCategoryRef);
	}

	return RootMenuChildren[TeamCityCategoryIndex];
}
#endif