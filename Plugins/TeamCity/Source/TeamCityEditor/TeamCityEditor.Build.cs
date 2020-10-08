	using UnrealBuildTool;

public class TeamCityEditor : ModuleRules
{
    public TeamCityEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.AddRange(
			new string[] {
				"TeamCityEditor/Private",
			}
			);
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				 "Core"
				,"CoreUObject"
				,"Engine"
				,"Projects"
			}
			);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				 "Slate"
				,"SlateCore"
                ,"DesktopPlatform"
                ,"SourceControl"
                ,"SourceControlWindows"
                ,"ContentBrowser"
                ,"XmlParser"
                ,"Http"
                ,"SourceControl"
            }
            );

		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd"
				,"LevelEditor"
				,"EditorStyle"
                ,"BlueprintGraph"
                ,"PropertyEditor"
				,"InputCore"
                ,"AssetRegistry"
                ,"AssetTools"
				,"ClassViewer"
				,"WorkspaceMenuStructure"
                ,"MessageLog"
            }
			);
		}
	}
}
