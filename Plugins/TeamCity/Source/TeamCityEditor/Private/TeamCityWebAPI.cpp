#include "TeamCityWebAPI.h"

FString FTeamCityWebAPI::Base64Encode(const FString& Source)
{
	TArray<uint8> ByteArray;
	FTCHARToUTF8 StringSrc = FTCHARToUTF8(Source.GetCharArray().GetData());
	ByteArray.Append((uint8*)StringSrc.Get(), StringSrc.Length());

	return FBase64::Encode(ByteArray);
}

static FString TeamCityCSRF;

void FTeamCityWebAPI::SetCSRF(const FString InCSRF)
{
	TeamCityCSRF = InCSRF;
}

FString FTeamCityWebAPI::GetCSRF()
{
	return TeamCityCSRF;
}

TSharedRef<IHttpRequest> FTeamCityWebAPI::PrepareGetRequest(const FString& TeamCityAPIFunction)
{
	TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	const UTeamCitySettings* Settings = GetMutableDefault<UTeamCitySettings>();
	FString URL(Settings->Server.ToString());
	URL += URL.EndsWith("/") ? "" : "/";
	URL += "httpAuth/" + TeamCityAPIFunction;
	HttpRequest->SetURL(URL);

	HttpRequest->SetVerb("GET");

	return HttpRequest;
}

bool FTeamCityWebAPI::ParseTeamCityResponse(class FXmlFile& XmlFile, FHttpResponsePtr HttpResponse)
{
	FString ResponseContent(HttpResponse->GetContentAsString());

	// strip out the xml header so that UE4 parses it correctly
	const FString XMLHeaderString(TEXT("<?xml version=\"1.0\" encoding=\"UTF - 8\" standalone=\"yes\"?>"));
	const int32 PosOfTeamCityXML = ResponseContent.Find(XMLHeaderString) + XMLHeaderString.Len() - 1;
	ResponseContent = ResponseContent.RightChop(PosOfTeamCityXML);

	return XmlFile.LoadFile(ResponseContent, EConstructMethod::ConstructFromBuffer);
}