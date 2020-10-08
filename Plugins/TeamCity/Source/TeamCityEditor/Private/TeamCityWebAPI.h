#pragma once

#include "CoreMinimal.h"
#include "TeamCitySettings.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "XmlParser.h"

namespace FTeamCityWebAPI
{
	FString Base64Encode(const FString& Source);

	void SetCSRF(const FString InCSRF);
	FString GetCSRF();

	TSharedRef<IHttpRequest> PrepareGetRequest(const FString& TeamCityAPIFunction);

	template<typename T1, typename T2>
	TSharedRef<IHttpRequest> PerformGetRequestRaw(const FString& TeamCityAPIFunction, T1&& TargetObj, T2&& TargetObjHandler)
	{
		TSharedRef<IHttpRequest> HttpRequest = PrepareGetRequest(TeamCityAPIFunction);

		HttpRequest->OnProcessRequestComplete().BindRaw(std::forward<T1>(TargetObj), std::forward<T2>(TargetObjHandler));
		HttpRequest->ProcessRequest();

		return HttpRequest;
	}

	template<typename T1, typename T2>
	TSharedRef<IHttpRequest> PerformGetRequestSP(const FString& TeamCityAPIFunction, T1&& TargetObj, T2&& TargetObjHandler)
	{
		TSharedRef<IHttpRequest> HttpRequest = PrepareGetRequest(TeamCityAPIFunction);

		HttpRequest->OnProcessRequestComplete().BindSP(std::forward<T1>(TargetObj), std::forward<T2>(TargetObjHandler));
		HttpRequest->ProcessRequest();

		return HttpRequest;
	}

	template<typename T1, typename T2>
	TSharedRef<IHttpRequest> PerformAuthenticatedGetRequest(const FString& TeamCityAPIFunction, T1&& TargetObj, T2&& TargetObjHandler)
	{
		TSharedRef<IHttpRequest> HttpRequest = PrepareGetRequest(TeamCityAPIFunction);

		const UTeamCitySettings* Settings = GetMutableDefault<UTeamCitySettings>();
		const FString AuthorizationString(Settings->Username.ToString() + ":" + Settings->Password.ToString());
		const FString EncodedAuthorizationString(Base64Encode(AuthorizationString));
		HttpRequest->SetHeader("Authorization: Basic", EncodedAuthorizationString);

		HttpRequest->OnProcessRequestComplete().BindRaw(std::forward<T1>(TargetObj), std::forward<T2>(TargetObjHandler));
		HttpRequest->ProcessRequest();

		return HttpRequest;
	}

	template<typename T1, typename T2>
	TSharedRef<IHttpRequest> PerformPostRequest(const FString& TeamCityAPIFunction, const FString& ContentType, const TArray<uint8>& Content, T1&& TargetObj, T2&& TargetObjHandler)
	{
		TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

		const UTeamCitySettings* Settings = GetMutableDefault<UTeamCitySettings>();
		FString URL(Settings->Server.ToString());

		const FString AuthorizationString(Settings->Username.ToString() + ":" + Settings->Password.ToString());
		const FString EncodedAuthorizationString(Base64Encode(AuthorizationString));
		HttpRequest->SetHeader("Authorization: Basic", EncodedAuthorizationString);
		HttpRequest->SetHeader("X-TC-CSRF-Token", GetCSRF()); 
		HttpRequest->SetHeader("Content-Type", ContentType);
		HttpRequest->SetHeader("Origin", URL);

		HttpRequest->SetContent(Content);

		URL += URL.EndsWith("/") ? "" : "/";
		URL += "httpAuth/" + TeamCityAPIFunction;
		HttpRequest->SetURL(URL);

		HttpRequest->SetVerb("POST");
		HttpRequest->OnProcessRequestComplete().BindRaw(std::forward<T1>(TargetObj), std::forward<T2>(TargetObjHandler));

		HttpRequest->ProcessRequest();
		return HttpRequest;
	}

	bool ParseTeamCityResponse(class FXmlFile& XmlFile, FHttpResponsePtr HttpResponse);
};