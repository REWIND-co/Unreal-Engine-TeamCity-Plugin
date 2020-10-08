#pragma once

#include "CoreMinimal.h"
#include "TeamCityEditor.h"
#include "TeamCityGetBuildConfigs.h"
#include "TeamCityFileSelectWidget.h"
#include "Input/Reply.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWindow.h"
#include "Widgets/Views/STreeView.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Misc/Optional.h"
#include "ISourceControlProvider.h"
#include "SourceControlOperations.h"
#include "HttpModule.h"

struct FRunPersonalBuildItem : public TSharedFromThis<FRunPersonalBuildItem>
{
	FRunPersonalBuildItem(BuildConfigPtr InBuildConfig)
		: BuildConfig(InBuildConfig)
		, CheckState(ECheckBoxState::Unchecked)
	{}

	const FString& GetID() const { return BuildConfig->GetID(); }
	const FString& GetName() const { return BuildConfig->GetName(); }

	ECheckBoxState GetCheckState() const;
	bool IsARunnableBuildConfig() const { return Children.Num() == 0; }

	void SetCheckState(ECheckBoxState State);
	void SetCheckState_RecursiveUp(ECheckBoxState State);
	void SetCheckState_RecursiveDown(ECheckBoxState State);

	BuildConfigPtr BuildConfig;
	ECheckBoxState CheckState;

	TSharedPtr<FRunPersonalBuildItem> Parent;
	TArray<TSharedPtr<FRunPersonalBuildItem>> Children;
};

typedef TSharedPtr<FRunPersonalBuildItem> RunPersonalBuildItemPtr;

class STeamCityRunPersonalBuildWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STeamCityRunPersonalBuildWidget) {}
		SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
		SLATE_ARGUMENT(TSharedPtr<STeamCityFileSelectWidget>, FileSelectWidget)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply OnRunPersonalBuild_Clicked();
	FReply OnClose_Clicked();

	bool IsRunPersonalBuildEnabled() const;

	void SetDescription(const FText& InDescription, ETextCommit::Type InCommitType);

	void OnGetChildren(RunPersonalBuildItemPtr InItem, TArray<RunPersonalBuildItemPtr>& OutChildren);
	TSharedRef<ITableRow> OnGenerateRow(RunPersonalBuildItemPtr InItem, const TSharedRef<STableViewBase>& OwnerTable);
	
	void SetTreeItemExpansionRecursive(RunPersonalBuildItemPtr TreeItem, bool bInExpansionState) const;
	TArray<RunPersonalBuildItemPtr> GetSelectedConfigs(RunPersonalBuildItemPtr TreeItem) const;

	void OnGetBuildConfigs(BuildConfigPtr RootBuildConfig);
	void OnPostedChanges(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);
	void OnTriggeredPersonalBuild(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

	TArray<uint8> GenerateTeamCityPatch(const TArray<struct FFileItem> Files) const;

	EVisibility GetProcessingPersonalBuildThrobberVisibility() const;

	void OnConnectionFlushComplete(ETeamCityConnectionState TeamCityConnectionState);

	TWeakPtr<SWindow> ParentWindowPtr;
	TWeakPtr<STeamCityFileSelectWidget> FileSelectWidget;

	TSharedPtr<STreeView<RunPersonalBuildItemPtr>> BuildConfigTreeView;
	TArray<RunPersonalBuildItemPtr> Items;

	TSharedPtr<FTeamCityGetBuildConfig> TeamCityGetBuildConfig;

	FText Description;

	bool IsProcessingPersonalBuild;
};
