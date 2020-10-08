#include "TeamCitySettings.h"

UTeamCitySettings::UTeamCitySettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Server(FText::FromString(TEXT("http://build.rewind.local:8090/")))
{}