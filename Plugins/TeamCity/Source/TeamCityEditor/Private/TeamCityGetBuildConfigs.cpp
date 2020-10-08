#include "TeamCityGetBuildConfigs.h"
#include "TeamCityWebAPI.h"
#include "ISourceControlModule.h"
#include "SourceControlOperations.h"
#include "SourceControlHelpers.h"
#include "CoreGlobals.h"

FTeamCityGetBuildConfig::FTeamCityGetBuildConfig()
	: IsConnectedToSourceControl(false)
{
	ISourceControlModule& SourceControlModule = ISourceControlModule::Get();
	SourceControlProviderChangedDelegateHandle = SourceControlModule.RegisterProviderChanged(FSourceControlProviderChanged::FDelegate::CreateRaw(this, &FTeamCityGetBuildConfig::OnSourceControlProviderChanged));
	SourceControlStateChangedDelegateHandle = SourceControlModule.GetProvider().RegisterSourceControlStateChanged_Handle(FSourceControlStateChanged::FDelegate::CreateRaw(this, &FTeamCityGetBuildConfig::Process));
}

FTeamCityGetBuildConfig::~FTeamCityGetBuildConfig()
{
	ISourceControlModule& SourceControlModule = ISourceControlModule::Get();
	SourceControlModule.GetProvider().UnregisterSourceControlStateChanged_Handle(SourceControlStateChangedDelegateHandle);
	SourceControlModule.UnregisterProviderChanged(SourceControlProviderChangedDelegateHandle);
}

void FTeamCityGetBuildConfig::OnSourceControlProviderChanged(ISourceControlProvider& OldProvider, ISourceControlProvider& NewProvider)
{
	OldProvider.UnregisterSourceControlStateChanged_Handle(SourceControlStateChangedDelegateHandle);
	SourceControlStateChangedDelegateHandle = NewProvider.RegisterSourceControlStateChanged_Handle(FSourceControlStateChanged::FDelegate::CreateRaw(this, &FTeamCityGetBuildConfig::Process));

	Process();
}

FString FTeamCityGetBuildConfig::FormatPathRelativeToSourceControl(const FString& RelativePath)
{
	return SourceControlRootPath + "/" + RelativePath;
}

void FTeamCityGetBuildConfig::Process()
{
	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	const bool IsNewConnectedToSourceControl = SourceControlProvider.IsAvailable() && SourceControlProvider.IsEnabled();

	// if we have just become connected
	if (IsConnectedToSourceControl == false && IsNewConnectedToSourceControl)
	{
		SourceControlProvider.Execute(ISourceControlOperation::Create<FConnect>(),
			EConcurrency::Asynchronous,
			FSourceControlOperationComplete::CreateSP(this, &FTeamCityGetBuildConfig::OnSourceControlConnectComplete));
	}

	// if we have just become disconnected
	if (IsConnectedToSourceControl && IsNewConnectedToSourceControl == false)
	{
		SourceControlRootPath = "";
		ValidBuildIDs.Empty();
		ValidBuildNames.Empty();
		ValidBuildProjectIDs.Empty();

		TeamCityGetBuildConfigDelegate.Broadcast(nullptr);
	}

	// if we remain connected then start the build query process
	if (IsConnectedToSourceControl && IsNewConnectedToSourceControl)
	{
		ProcessBuildTypesQuery();
	}

	IsConnectedToSourceControl = IsNewConnectedToSourceControl;
}

ESupportedSourceControlProvider FTeamCityGetBuildConfig::GetSupportedSourceControlProvider() const
{
	const ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	const bool IsPerforce = SourceControlProvider.GetName() == FName(TEXT("Perforce"));

	return IsPerforce ? ESupportedSourceControlProvider::Perforce : ESupportedSourceControlProvider::Unknown;
}

void FTeamCityGetBuildConfig::OnSourceControlConnectComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	switch (GetSupportedSourceControlProvider())
	{
		case ESupportedSourceControlProvider::Perforce:
		{
			const FString StreamString(TEXT("Stream ")); // if found, syntax will be "Stream //depot/stream/"

			const FSourceControlResultInfo& ResultInfo = InOperation->GetResultInfo();
			const FText* StreamInfo = ResultInfo.InfoMessages.FindByPredicate([StreamString](const FText& Msg)
			{
				return Msg.ToString().Contains(StreamString);
			});

			if (StreamInfo != nullptr)
			{
				// if found, syntax will be "Stream //depot/stream/"
				const FString StreamInfoString(StreamInfo->ToString());
				const int32 PosOfP4StreamPath = StreamInfoString.Find(StreamString) + StreamString.Len();

				// get the server out of the ini file as this is the only means of doing so that is exposed to plugins
				FString P4Server;
				const FString& IniFile = SourceControlHelpers::GetSettingsIni();
				const FString SourceControlSettingsSection(TEXT("PerforceSourceControl.PerforceSourceControlSettings"));
				GConfig->GetString(*SourceControlSettingsSection, TEXT("Port"), P4Server, IniFile);
				check(P4Server.IsEmpty() == false); // If this fires, then it is likely the config section for P4 settings has been renamed/changed

				SourceControlRootPath = FString(TEXT("perforce://")) + P4Server + FString("://") + StreamInfoString.RightChop(PosOfP4StreamPath); 
			}
			break;
		}
		default:
		case ESupportedSourceControlProvider::Unknown:
		{
			// This source control provider is currently unsupported! Please add functionality here for caching the source control root path for this provider.
			check(0);
			break;
		}
	}

	ProcessBuildTypesQuery();
}

bool FTeamCityGetBuildConfig::DoesBuildConfigMatchSourceControlPath(const FXmlNode* VCSPropertiesRoot, const TMap<FString, FString>& BuildTypeProjectParameters) const
{
	bool BuildConfigMatchesSourceControlPath = false;

	if(IsConnectedToSourceControl)
	{
		switch (GetSupportedSourceControlProvider())
		{
			case ESupportedSourceControlProvider::Perforce:
			{
				const TArray<FXmlNode*>& Properties = VCSPropertiesRoot->GetChildrenNodes();
				for (auto& Property : Properties)
				{
					if (Property->GetAttribute("name") == TEXT("stream"))
					{
						// we've found the p4 stream reference for this teamcity VCS-Root
						FString P4StreamPath(Property->GetAttribute(TEXT("value")));
	
						// if this value uses parameters, try and replace with a match in our previously generated parameter map
						for (auto& ParameterTuple : BuildTypeProjectParameters)
						{
							P4StreamPath = P4StreamPath.Replace(*ParameterTuple.Key, *ParameterTuple.Value);
						}
	
						if (SourceControlRootPath.Contains(P4StreamPath))
						{
							BuildConfigMatchesSourceControlPath = true;
							break;
						}
					}
				}
				break;
			}
			default:
			case ESupportedSourceControlProvider::Unknown:
			{
				// This source control provider is currently unsupported! Please add functionality here for validating 
				// this set of build config properties defines a match with the current source control root path.
				check(0);
				break;
			}
		}
		}

	return BuildConfigMatchesSourceControlPath;
}

void FTeamCityGetBuildConfig::ProcessBuildTypesQuery()
{
	const FString TeamCityBuildTypeQueryString(TEXT("app/rest/buildTypes?fields=buildType(id,name,project(name,id,archived),parameters(property(name,value)),vcs-root-entries(vcs-root-entry(vcs-root(id,name,properties(property(name,value))))))"));
	FTeamCityWebAPI::PerformGetRequestSP(TeamCityBuildTypeQueryString, this, &FTeamCityGetBuildConfig::OnGetBuildTypes);
}

void FTeamCityGetBuildConfig::OnGetBuildTypes(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	const bool GotValidResponse = bSucceeded && HttpResponse->GetResponseCode() == 200;
	if (GotValidResponse)
	{
		FXmlFile XmlFile;
		const bool XmlParseSuccess = FTeamCityWebAPI::ParseTeamCityResponse(XmlFile, HttpResponse);

		if (XmlParseSuccess)
		{
			ProcessBuildTypesXml(XmlFile, ValidBuildIDs, ValidBuildNames, ValidBuildProjectIDs);

			// now fire off a call to retrieve those builds which do not support personal builds so we can filter them out (TC report they currently
			// do not have a means of exclusively querying for builds that *do* support personal builds, so we have to do it this way.
			const FString TeamCityUnsupprtedBuildTypeQueryString(TEXT("app/rest/buildTypes?locator=setting:(name:allowPersonalBuildTriggering,value:false)&fields=buildType(id)"));
			FTeamCityWebAPI::PerformGetRequestSP(TeamCityUnsupprtedBuildTypeQueryString, this, &FTeamCityGetBuildConfig::OnGetUnsupportedBuildTypes);
		}
	}
	else
	{
		TeamCityGetBuildConfigDelegate.Broadcast(nullptr);
	}
}

void FTeamCityGetBuildConfig::OnGetUnsupportedBuildTypes(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	const bool GotValidResponse = bSucceeded && HttpResponse->GetResponseCode() == 200;
	if (GotValidResponse)
	{
		FXmlFile XmlFile;
		const bool XmlParseSuccess = FTeamCityWebAPI::ParseTeamCityResponse(XmlFile, HttpResponse);

		if (XmlParseSuccess)
		{
			ProcessUnsupportedBuildTypesXml(XmlFile, ValidBuildIDs, ValidBuildNames, ValidBuildProjectIDs);

			// now that we have filtered out unsupported builds, get the set of all projects so that we can build up our tree hierarchy
			const FString TeamCityProjectsQueryString(TEXT("app/rest/projects?fields=project(id,name,parentProjectId)"));
			FTeamCityWebAPI::PerformGetRequestSP(TeamCityProjectsQueryString, this, &FTeamCityGetBuildConfig::OnGetTeamCityProjects);
		}
	}
	else
	{
		TeamCityGetBuildConfigDelegate.Broadcast(nullptr);
	}
}

void FTeamCityGetBuildConfig::ProcessBuildTypesXml(const FXmlFile& XmlFile, TArray<FString> &InValidBuildIDs, TArray<FString> &InValidBuildNames, TArray<FString> &InValidBuildProjectIDs) const
{
	InValidBuildIDs.Empty();
	InValidBuildNames.Empty();
	InValidBuildProjectIDs.Empty();

	// Root node should include count
	const FXmlNode* RootNode = XmlFile.GetRootNode();

	TArray<FXmlNode*> BuildTypeNodes;
	if (RootNode != nullptr)
	{
		BuildTypeNodes = RootNode->GetChildrenNodes();
	}

	for (auto& BuildTypeNode : BuildTypeNodes)
	{
		// get the build's project name
		FString ProjectID;
		{
			const FXmlNode* ProjectRoot = BuildTypeNode->FindChildNode(TEXT("project"));
			check(ProjectRoot != nullptr);

			const FString Archived(ProjectRoot->GetAttribute(TEXT("archived")));
			if (Archived == TEXT("true")) { continue; }

			ProjectID = ProjectRoot->GetAttribute(TEXT("id"));
		}

		// build up a set of all parameters for this project
		TMap<FString, FString> BuildTypeProjectParameters;
		{
			const FXmlNode* ParametersRoot = BuildTypeNode->FindChildNode(TEXT("parameters"));
			check(ParametersRoot != nullptr);

			const TArray<FXmlNode*>& Parameters = ParametersRoot->GetChildrenNodes();

			for (auto& BuildTypeProjectParametersNode : Parameters)
			{
				const FString ParameterEnvironmentVariable = FString(TEXT("%")) + BuildTypeProjectParametersNode->GetAttribute(TEXT("name")) + FString(TEXT("%")); //TC env vars are referenced in the form %MyEnvVar%
				BuildTypeProjectParameters.Add(ParameterEnvironmentVariable, BuildTypeProjectParametersNode->GetAttribute(TEXT("value")));
			}
		}

		// iterate over all teamcity's vcs root nodes for this build type and try to find a match with our UE4 source control root path
		const FXmlNode* VCSEntriesRoot = BuildTypeNode->FindChildNode(TEXT("vcs-root-entries"));
		check(VCSEntriesRoot != nullptr);

		const TArray<FXmlNode*>& VCSEntries = VCSEntriesRoot->GetChildrenNodes();
		for (auto& VCSEntry : VCSEntries)
		{
			const TArray<FXmlNode*>& VCSRoots = VCSEntry->GetChildrenNodes();
			for (auto& VCSRoot : VCSRoots)
			{
				const FXmlNode* VCSPropertiesRoot = VCSRoot->FindChildNode(TEXT("properties"));
				check(VCSPropertiesRoot != nullptr);

				if (DoesBuildConfigMatchSourceControlPath(VCSPropertiesRoot, BuildTypeProjectParameters))
				{
					// we've found a match for the source control root path UE4 is using, so cache in the arrays
					InValidBuildIDs.Emplace(BuildTypeNode->GetAttribute(TEXT("id")));
					InValidBuildNames.Emplace(BuildTypeNode->GetAttribute(TEXT("name")));
					InValidBuildProjectIDs.Emplace(ProjectID);
				}
			}
		}
	}
}

void FTeamCityGetBuildConfig::ProcessUnsupportedBuildTypesXml(const FXmlFile& XmlFile, TArray<FString> &InValidBuildIDs, TArray<FString> &InValidBuildNames, TArray<FString> &InValidBuildProjectIDs) const
{
	// Root node should include count
	const FXmlNode* RootNode = XmlFile.GetRootNode();

	TArray<FXmlNode*> BuildTypeNodes;
	if (RootNode != nullptr)
	{
		BuildTypeNodes = RootNode->GetChildrenNodes();
	}

	for (auto& BuildTypeNode : BuildTypeNodes)
	{
		const FString BuildID(BuildTypeNode->GetAttribute(TEXT("id")));
		int32 BuildIDIndex = -1;
		bool FoundID = ValidBuildIDs.Find(BuildID, BuildIDIndex);
		if (FoundID)
		{
			InValidBuildIDs.RemoveAt(BuildIDIndex);
			InValidBuildNames.RemoveAt(BuildIDIndex);
			InValidBuildProjectIDs.RemoveAt(BuildIDIndex);
		}
	}
}

void FTeamCityGetBuildConfig::OnGetTeamCityProjects(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded) const
{
	// note: this algorithm works because teamcity returns a list of projects in hierarchically descending order. if this functionality changes, then this algorithm will break

	BuildConfigPtr RootBuildConfig;
	const bool GotValidResponse = bSucceeded && HttpResponse->GetResponseCode() == 200;
	if (GotValidResponse)
	{
		FXmlFile XmlFile;
		const bool XmlParseSuccess = FTeamCityWebAPI::ParseTeamCityResponse(XmlFile, HttpResponse);
		if (XmlParseSuccess)
		{
			// create our set of project build config items
			const FXmlNode* RootNode = XmlFile.GetRootNode();
			check(RootNode->GetTag() == TEXT("projects"));

			const TArray<FXmlNode*>& Projects = RootNode->GetChildrenNodes();
			for (auto& Project : Projects)
			{
				FString Id(Project->GetAttribute(TEXT("id")));
				FString Name(Project->GetAttribute(TEXT("name")));
				FString ParentProjectID(Project->GetAttribute(TEXT("parentProjectId")));

				BuildConfigPtr ParentProjectConfig = FindBuildConfigByID(ParentProjectID, RootBuildConfig);
				BuildConfigPtr ProjectConfig = BuildConfigPtr(new FBuildConfigItem(Id, Name, ParentProjectConfig));
				if (ParentProjectConfig.IsValid()) { ParentProjectConfig->GetChildren().Emplace(ProjectConfig); }

				if (ParentProjectConfig.IsValid() == false) { RootBuildConfig = ProjectConfig; }
			}
		}
	}

	if (RootBuildConfig.IsValid())
	{
		// now strip any leaf node configs (which are still just projects at this stage) whose IDs weren't captured when we identified which had valid VCS roots
		StripInvalidProjects(RootBuildConfig, ValidBuildProjectIDs);

		// now we can insert our previously identified build configs
		InjectBuildConfigs(RootBuildConfig, ValidBuildIDs, ValidBuildNames, ValidBuildProjectIDs);
	}

	TeamCityGetBuildConfigDelegate.Broadcast(RootBuildConfig);
}

BuildConfigPtr FTeamCityGetBuildConfig::FindBuildConfigByID(const FString& ID, const BuildConfigPtr RootBuildConfig) const
{
	// recursively traverse the tree till we find an ID match, otherwise return null
	BuildConfigPtr Result;
	if (RootBuildConfig.IsValid())
	{
		if (RootBuildConfig->GetID() == ID)
		{
			Result = RootBuildConfig;
		}
		else
		{
			for (auto& Child : RootBuildConfig->GetChildren()) { Result = FindBuildConfigByID(ID, Child); }
		}
	}
	return Result;
}

void FTeamCityGetBuildConfig::StripInvalidProjects(const BuildConfigPtr RootBuildConfig, const TArray<FString>& InValidBuildProjectIDs) const
{
	TArray<TSharedPtr<FBuildConfigItem>>& RootBuildConfigChildren = RootBuildConfig->GetChildren();
	for (int32 i = RootBuildConfigChildren.Num() - 1; i >= 0; i--)
	{
		BuildConfigPtr Child = RootBuildConfigChildren[i];
		if (Child->IsARunnableBuildConfig() == false)
		{
			StripInvalidProjects(Child, InValidBuildProjectIDs);
		}
		
		if (Child->IsARunnableBuildConfig())
		{
			// we've hit a leaf node, so either its a project that we've previously identified as being a VCS match, or its not, in which case strip it
			if (InValidBuildProjectIDs.Contains(Child->GetID()) == false)
			{
				RootBuildConfigChildren.RemoveAt(i);
			}
		}
	}
}

void FTeamCityGetBuildConfig::InjectBuildConfigs(const BuildConfigPtr RootBuildConfig, const TArray<FString>& InValidBuildIDs, const TArray<FString>& InValidBuildNames, const TArray<FString>& InValidBuildProjectIDs) const
{
	for (auto& Child : RootBuildConfig->GetChildren())
	{
		InjectBuildConfigs(Child, InValidBuildIDs, InValidBuildNames, InValidBuildProjectIDs);

		if (Child->IsARunnableBuildConfig())
		{
			check(InValidBuildProjectIDs.Contains(Child->GetID())); // since this should always running after pruning invalid projects, this should always be true
			for (int32 ProjectIDsIndex = 0; ProjectIDsIndex < InValidBuildProjectIDs.Num(); ProjectIDsIndex++)
			{
				// we've found a parent project match for one of our build configs, so go ahead and add it to the tree
				if (Child->GetID() == InValidBuildProjectIDs[ProjectIDsIndex])
				{
					BuildConfigPtr BuildConfig = BuildConfigPtr(new FBuildConfigItem(InValidBuildIDs[ProjectIDsIndex], InValidBuildNames[ProjectIDsIndex], Child));
					Child->GetChildren().Emplace(BuildConfig);
				}
			}
		}
	}
}