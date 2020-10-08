#pragma once

#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/EngineBaseTypes.h"

struct FTeamCityBuildNotification : public FTickableEditorObject
{	
	FTeamCityBuildNotification();
	virtual ~FTeamCityBuildNotification() {};

	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return bIsTickEnabled; }
	virtual TStatId GetStatId() const override;

	TWeakPtr<SNotificationItem> NotificationItemPtr;
	FString BuildID;
	FString HyperlinkURL;

	void OnReceiveBuildLog(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

	void QueryTeamCityPersonalBuildStatus();
	void OnQueryTeamCityPersonalBuildStatus(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);
	void OnCanceledPersonalBuild(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);
	void OnPersonalBuildFailure();
	void OnPersonalBuildSuccessful();
	void ResolveTeamCityBuildNotification(const FText& InText, SNotificationItem::ECompletionState InCompletionState);
	
	float TimeElapsedSinceLastQuery;
	float QueryInterval;
	bool bIsTickEnabled;

	FString TeamCityBuildLog;
};

struct FTeamCityBuildNotificationsHelper
{
public:
	FTeamCityBuildNotificationsHelper();
	~FTeamCityBuildNotificationsHelper();

	static void AddTeamCityBuildNotification(const FText& NotificationText, const FString& HyperlinkURL = "", const FString& BuildID = "");
};