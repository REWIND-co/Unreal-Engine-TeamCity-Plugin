#include "TeamCityEditor.h"
#include "TeamCityEditorPCH.h"
#include "TeamCityWebAPI.h"

#if WITH_EDITOR
#include "TeamCityStyle.h"
#include "TeamCitySettings.h"
#endif

#define LOCTEXT_NAMESPACE "LogTeamCityEditor"
DEFINE_LOG_CATEGORY(LogTeamCityEditor);

FTeamCityEditorModule::FTeamCityEditorModule()
	: ConnectionState(ETeamCityConnectionState::Disconnected)
{
#if WITH_EDITOR
	if(IsRunningCommandlet() == false && IsRunningGame() == false)
	{
		FlushConnection();
	}
#endif
}

FTeamCityEditorModule& FTeamCityEditorModule::LoadModuleChecked()
{
	return FModuleManager::LoadModuleChecked<FTeamCityEditorModule>("TeamCityEditor");
}

void FTeamCityEditorModule::StartupModule()
{
#if WITH_EDITOR
	if (IsRunningCommandlet() == false && IsRunningGame() == false)
	{
		FTeamCityStyle::Create();
		Tab = MakeShareable(new FTeamCityTabSpawner);
		Tab->Create();

		// TC cookies come back with no expiration time on them, and by default last for 1 minute, so
		// we periodically re-authenticate/reform the connection, renewing our CSRF token and cookie
		const float UpdateConnectionPeriod = 30.0f;
		UpdateConnectionDelegate = FTickerDelegate::CreateRaw(this, &FTeamCityEditorModule::UpdateConnection);
		UpdateConnectionDelegateHandler = FTicker::GetCoreTicker().AddTicker(UpdateConnectionDelegate, UpdateConnectionPeriod);
	}
#endif
}

void FTeamCityEditorModule::ShutdownModule()
{
#if WITH_EDITOR
	if (IsRunningCommandlet() == false && IsRunningGame() == false)
	{
		Tab->Destroy();
		FTeamCityStyle::Destroy();
	}
#endif
}
void FTeamCityEditorModule::FlushConnection()
{
	// @SamB: Brute force, but since UE4 provides no means of flushing curl cookies we are forced to recreate our curl runtime, and the only means
	// of doing this in a thread safe manner WRT currently in flight multithreaded http requests is to reload the entire http module.
	FModuleManager::Get().UnloadModule("HTTP");
	FModuleManager::Get().LoadModule("HTTP");

	Authenticate(this, &FTeamCityEditorModule::OnFlushConnectionAuthenticationRequestComplete);
}

bool FTeamCityEditorModule::UpdateConnection(float DeltaTime)
{
	// if we are already connected, periodically refresh the connection, but don't do it otherwise as teamcity will prevent access if repeated bad login attempts are made
	if (GetConnectionState() == ETeamCityConnectionState::Connected)
	{
		Authenticate(this, &FTeamCityEditorModule::OnUpdateConnectionAuthenticationRequestComplete);
	}
	return true;
}

void FTeamCityEditorModule::OnUpdateConnectionAuthenticationRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	const bool GotSuccessResponseCode = HttpResponse->GetResponseCode() == 200;
	ConnectionState = bSucceeded && GotSuccessResponseCode ? ETeamCityConnectionState::Connected : ETeamCityConnectionState::Disconnected;

	if (ConnectionState == ETeamCityConnectionState::Connected)
	{
		const FString CSRF(HttpResponse->GetContentAsString());
		FTeamCityWebAPI::SetCSRF(CSRF);
	}
	else
	{
		UE_LOG(LogTeamCityEditor, Log, TEXT("Failed to connect to TeamCity server with response code %i, error message: %s"), HttpResponse->GetResponseCode(), *HttpResponse->GetContentAsString());
	}
}

void FTeamCityEditorModule::OnFlushConnectionAuthenticationRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	OnUpdateConnectionAuthenticationRequestComplete(HttpRequest, HttpResponse, bSucceeded);
	TeamCityOnConnectionFlushComplete.Broadcast(ConnectionState);
}

void FTeamCityEditorModule::OpenHyperlink(const FString URL)
{
	FPlatformProcess::LaunchURL(*URL, NULL, NULL);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTeamCityEditorModule, TeamCityEditor)
