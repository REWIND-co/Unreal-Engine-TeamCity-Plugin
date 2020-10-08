#pragma once
#include "CoreMinimal.h"
#include "HttpModule.h"
#include "XmlFile.h"
#include "ISourceControlProvider.h"

enum class ESupportedSourceControlProvider : uint8
{
	Unknown,
	Perforce
};

struct FBuildConfigItem : public TSharedFromThis<FBuildConfigItem>
{
	FBuildConfigItem(const FString& InID, const FString& InName)
		: ID(InID)
		, Name(InName)
	{}

	FBuildConfigItem(const FString& InID, const FString& InName, TSharedPtr<FBuildConfigItem> InParent)
		: ID(InID)
		, Name(InName)
		, Parent(InParent)
	{}

	const FString& GetID() const { return ID; }
	const FString& GetName() const { return Name; }
	bool IsARunnableBuildConfig() const { return Children.Num() == 0; }
	TArray<TSharedPtr<FBuildConfigItem>>& GetChildren() { return Children; }

protected:
	FString ID;
	FString Name;

	TSharedPtr<FBuildConfigItem> Parent;
	TArray<TSharedPtr<FBuildConfigItem>> Children;
};

typedef TSharedPtr<FBuildConfigItem> BuildConfigPtr;

DECLARE_MULTICAST_DELEGATE_OneParam(FTeamCityGetBuildConfigDelegate, TSharedPtr<FBuildConfigItem> /*RootBuildConfig*/);

class FTeamCityGetBuildConfig : public TSharedFromThis<FTeamCityGetBuildConfig>
{
public:
	FTeamCityGetBuildConfig();
	~FTeamCityGetBuildConfig();

	void Process();

	FTeamCityGetBuildConfigDelegate& OnGetBuildsComplete() { return TeamCityGetBuildConfigDelegate; }
	FString FormatPathRelativeToSourceControl(const FString& RelativePath);

private:
	ESupportedSourceControlProvider GetSupportedSourceControlProvider() const;

	void OnSourceControlProviderChanged(ISourceControlProvider& OldProvider, ISourceControlProvider& NewProvider);
	void OnSourceControlConnectComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);

	void ProcessBuildTypesQuery();

	void OnGetBuildTypes(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);
	void OnGetUnsupportedBuildTypes(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);
	void ProcessBuildTypesXml(const FXmlFile& XmlFile, TArray<FString> &ValidBuildIDs, TArray<FString> &ValidBuildNames, TArray<FString> &ValidBuildProjectIDs) const;
	void ProcessUnsupportedBuildTypesXml(const FXmlFile& XmlFile, TArray<FString> &ValidBuildIDs, TArray<FString> &ValidBuildNames, TArray<FString> &ValidBuildProjectIDs) const;

	bool DoesBuildConfigMatchSourceControlPath(const FXmlNode* VCSPropertiesRoot, const TMap<FString, FString>& BuildTypeProjectParameters) const;

	void OnGetTeamCityProjects(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded) const;

	BuildConfigPtr FindBuildConfigByID(const FString& ID, const BuildConfigPtr RootBuildConfig) const;
	void StripInvalidProjects(const BuildConfigPtr RootBuildConfig, const TArray<FString>& InValidBuildProjectIDs) const;
	void InjectBuildConfigs(const BuildConfigPtr RootBuildConfig, const TArray<FString>& InValidBuildIDs, const TArray<FString>& InValidBuildNames, const TArray<FString>& InValidBuildProjectIDs) const;

	FTeamCityGetBuildConfigDelegate TeamCityGetBuildConfigDelegate;

	TArray<FString> ValidBuildIDs;
	TArray<FString> ValidBuildNames;
	TArray<FString> ValidBuildProjectIDs;

	FString SourceControlRootPath;
	bool IsConnectedToSourceControl;
	FDelegateHandle SourceControlProviderChangedDelegateHandle;
	FDelegateHandle SourceControlStateChangedDelegateHandle;
};