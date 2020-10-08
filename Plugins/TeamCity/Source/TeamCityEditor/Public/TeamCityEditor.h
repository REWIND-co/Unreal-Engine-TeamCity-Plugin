#pragma once

#include "Modules/ModuleManager.h"
#include "CoreMinimal.h"
#include "HttpModule.h"
#include "Containers/Ticker.h"
#include "TeamCityWebAPI.h"

#include "ISourceControlProvider.h"
#include "SourceControlOperations.h"

#if WITH_EDITOR
#include "TeamCityTabSpawner.h"
#endif

DECLARE_LOG_CATEGORY_EXTERN(LogTeamCityEditor, Log, All);

enum class ETeamCityConnectionState : uint8
{
	Disconnected,
	Connecting,
	Connected
};

DECLARE_MULTICAST_DELEGATE_OneParam(FTeamCityOnConnectionFlushComplete, ETeamCityConnectionState /*TeamCityConnectionState*/);

class FTeamCityEditorModule : public IModuleInterface
{
public:
	FTeamCityEditorModule();

	static FTeamCityEditorModule& LoadModuleChecked();

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	ETeamCityConnectionState GetConnectionState() const { return ConnectionState; }

	void FlushConnection();
	FTeamCityOnConnectionFlushComplete& OnConnectionFlushComplete() { return TeamCityOnConnectionFlushComplete; }

	static void OpenHyperlink(const FString URL);
	
private:
	template<typename T1, typename T2>
	void Authenticate(T1&& TargetObj, T2&& TargetObjHandler)
	{
		ConnectionState = ETeamCityConnectionState::Connecting;
		FTeamCityWebAPI::PerformAuthenticatedGetRequest("authenticationTest.html?csrf", TargetObj, TargetObjHandler);
	}

	bool UpdateConnection(float DeltaTime);
	void OnUpdateConnectionAuthenticationRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);
	void OnFlushConnectionAuthenticationRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

#if WITH_EDITOR
	TSharedPtr<FTeamCityTabSpawner> Tab;
#endif

	ETeamCityConnectionState ConnectionState;
	FTeamCityOnConnectionFlushComplete TeamCityOnConnectionFlushComplete;

	FTickerDelegate UpdateConnectionDelegate;
	FDelegateHandle UpdateConnectionDelegateHandler;
};