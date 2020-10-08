#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Misc/Guid.h"
#include "TeamCitySettings.generated.h"

UCLASS(Config=EditorPerProjectUserSettings)
class UTeamCitySettings : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Config)
	FText Server;

	UPROPERTY(Config)
	FText Username;

	UPROPERTY(Config)
	FText Password;
};