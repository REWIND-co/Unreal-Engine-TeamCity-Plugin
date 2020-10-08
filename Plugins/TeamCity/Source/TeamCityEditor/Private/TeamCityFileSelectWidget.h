#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Misc/Optional.h"
#include "TeamCityFileItem.h"
#include "ISourceControlProvider.h"
#include "SourceControlOperations.h"

class STeamCityFileSelectWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STeamCityFileSelectWidget)
	{}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	bool CanRefresh() const;
	void RefreshItems();
	bool IsRefreshingFiles() const { return RefreshingFiles; }

	TArray<FFileItem> GetSelectedFiles(FFileItem* RootItem = nullptr) const;

private:
	void OnGetChildren(FilePtr InItem, TArray<FilePtr>& OutChildren);
	TSharedRef<ITableRow> OnGenerateRow(FilePtr InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void SetTreeItemExpansionRecursive(FilePtr TreeItem, bool bInExpansionState) const;

	void CopyCheckState(FilePtr Dest, FilePtr Source) const;
	void OnUpdatedSourceControlStatus(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);
	void OnUpdatedSourceControlHistory(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);

	TArray<FSourceControlStateRef> GetPendingSourceControlItems() const;

	TSharedPtr<STreeView<FilePtr>> FileSelectTreeView;
	TArray<FilePtr> Items;

	bool RefreshingFiles;
	float TimeSinceLastRefresh;
	bool ConnectedToSourceControl;
};
