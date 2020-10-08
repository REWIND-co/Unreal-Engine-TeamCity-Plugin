#include "TeamCityFileSelectWidget.h"
#include "TeamCityStyle.h"
#include "TeamCityEditorPCH.h"
#include "ISourceControlModule.h"
#include "HAL/PlatformFilemanager.h"

#if WITH_EDITOR
#include "EditorStyleSet.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Images/SImage.h"
#endif

void STeamCityFileSelectWidget::Construct(const FArguments& InArgs)
{
#if WITH_EDITOR
	ChildSlot
	[
		SNew(SBorder)
		.Padding(FMargin(3))
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
		[
			SAssignNew(FileSelectTreeView, STreeView<FilePtr>)
			.TreeItemsSource(&Items)
			.OnGenerateRow(this, &STeamCityFileSelectWidget::OnGenerateRow)
			.OnGetChildren(this, &STeamCityFileSelectWidget::OnGetChildren)
			.SelectionMode(ESelectionMode::None)
		]
	];
#endif
	RefreshingFiles = false;
	TimeSinceLastRefresh = 0.0f;
	ConnectedToSourceControl = false;
	SetCanTick(true);
}

void STeamCityFileSelectWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	bool ShouldRefreshItems = false;
	TimeSinceLastRefresh += InDeltaTime;

	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	const bool NewConnectedToSourceControl = SourceControlProvider.IsAvailable() && SourceControlProvider.IsEnabled();

	if (NewConnectedToSourceControl != ConnectedToSourceControl)
	{
		ConnectedToSourceControl = NewConnectedToSourceControl;

		if (ConnectedToSourceControl == false)
		{
			Items.Empty();
			FileSelectTreeView->RequestTreeRefresh();
		}
		else { ShouldRefreshItems = true; }

	}
	else
	{
		static const float RefreshPeriod = 30.0f;
		if (TimeSinceLastRefresh >= RefreshPeriod) { ShouldRefreshItems = true; }
	}

	if (ShouldRefreshItems)
	{
		RefreshItems();
	}
}

void STeamCityFileSelectWidget::CopyCheckState(FilePtr Dest, FilePtr Source) const
{
	// migrate check states across to source ptr and then replace dest
	for (auto& Child : Dest->Children)
	{
		// find a match in dest
		const FString& SourceChildPath = Child->Path;
		FilePtr* Result = Source->Children.FindByPredicate([SourceChildPath](const FilePtr& Element)
		{
			return SourceChildPath == Element->Path;
		});

		if (Result != nullptr)
		{
			const ECheckBoxState CheckState = (*Result)->GetCheckState();
			const bool IsALeafNode = (*Result)->Children.Num() == 0;
			if (CheckState == ECheckBoxState::Checked && IsALeafNode)
			{
				Child->SetCheckState(CheckState);
			}
			CopyCheckState(Child, *Result);
		}
	}
}

void STeamCityFileSelectWidget::OnGetChildren(FilePtr InItem, TArray<FilePtr>& OutChildren)
{
	OutChildren = InItem->Children;
}

TSharedRef<ITableRow> STeamCityFileSelectWidget::OnGenerateRow(FilePtr InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	FString PathString(InItem->Path);	
	if(InItem->Parent != nullptr)
	{
		//non-root paths just show their local name
		const int32 LastSlashPos = InItem->Path.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		PathString = FString(InItem->Path.RightChop(LastSlashPos + 1));
	}

	return
		SNew(STableRow<FilePtr>, OwnerTable)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(0.0f, 0.0f, 1.0f, 0.0f)
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([InItem] { return InItem->GetCheckState(); })
					.OnCheckStateChanged_Lambda([InItem](ECheckBoxState State) { InItem->SetCheckState(State); })
				]
				+ SHorizontalBox::Slot()
				.Padding(0.0f, 0.0f, 6.0f, 0.0f)
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
					.MaxDesiredWidth(12)
					.MaxDesiredHeight(12)
					[
						SNew(SImage)
						.Image(InItem.Get(), &FFileItem::GetSCCStateImage)
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(PathString))
				]
			]
		];
}

void STeamCityFileSelectWidget::SetTreeItemExpansionRecursive(FilePtr TreeItem, bool bInExpansionState) const
{
	FileSelectTreeView->SetItemExpansion(TreeItem, bInExpansionState);

	for (auto&& Child : TreeItem->Children)
	{
		SetTreeItemExpansionRecursive(Child, bInExpansionState);
	}
}

class FFindFilesVisitor : public IPlatformFile::FDirectoryVisitor
{
public:
	IPlatformFile&		PlatformFile;
	TArray<FString>&	FoundFiles;
	TArray<FString>		FileExtensions;

	FFindFilesVisitor(IPlatformFile& InPlatformFile, TArray<FString>& InFoundFiles, TArray<FString>& InFileExtensions)
		: PlatformFile(InPlatformFile)
		, FoundFiles(InFoundFiles)
		, FileExtensions(InFileExtensions)
	{}

	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
	{
		if (!bIsDirectory)
		{
			bool ValidFile = false;
			for (const auto& FileExtension : FileExtensions)
			{
				int FileExtensionLen = FileExtension.Len();
				if (FileExtensionLen > 0)
				{
					int32 FileNameLen = FCString::Strlen(FilenameOrDirectory);
					if (FileNameLen < FileExtensionLen || FCString::Strcmp(&FilenameOrDirectory[FileNameLen - FileExtensionLen], *FileExtension) == 0)
					{
						ValidFile = true;
					}
				}
			}

			if (ValidFile)
			{
				FoundFiles.Emplace(FString(FilenameOrDirectory));
			}
		}
		return true;
	}
};

FilePtr AddItem(const FString& InPath, TArray<FilePtr>& DirectoryItems, EChangeType InChangeType)
{
	FString ParentPath, ChildPath;
	InPath.Split(TEXT("/"), &ParentPath, &ChildPath, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

	check(ParentPath.IsEmpty() == false); // fail if recursion has broken

	const FilePtr* ParentItemPtr = DirectoryItems.FindByPredicate(
		[ParentPath](const FilePtr& FileSelectItem)
	{
		return (FileSelectItem->Path == ParentPath);
	});

	if (ParentItemPtr == nullptr)
	{
		FilePtr ParentItem = AddItem(ParentPath, DirectoryItems, EChangeType::None);
		DirectoryItems.Emplace(ParentItem);
		ParentItemPtr = &DirectoryItems.Last();
	}

	FilePtr NewItem = FilePtr(new FFileItem(InPath, *ParentItemPtr, InChangeType));
	(*ParentItemPtr)->Children.Emplace(NewItem);
	return NewItem;
}

void STeamCityFileSelectWidget::RefreshItems()
{
	RefreshingFiles = true;
	TimeSinceLastRefresh = 0.0f;

	TArray<FString> UpdateStatusPaths;
	UpdateStatusPaths.Add(FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir()));
	UpdateStatusPaths.Add(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()));
	UpdateStatusPaths.Add(FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir()));
	UpdateStatusPaths.Add(FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir()));

	if (ConnectedToSourceControl) 
	{
		// first perform a fast update status request for all files in the project
		TSharedRef<FUpdateStatus, ESPMode::ThreadSafe> UpdateStatusOperation = ISourceControlOperation::Create<FUpdateStatus>();
		ISourceControlModule::Get().GetProvider().Execute(UpdateStatusOperation, UpdateStatusPaths, EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateSP(this, &STeamCityFileSelectWidget::OnUpdatedSourceControlStatus));
	}
}

void STeamCityFileSelectWidget::OnUpdatedSourceControlStatus(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	const TArray<FSourceControlStateRef> PendingItems(GetPendingSourceControlItems());

	// perform a slower update history request on all items we've identified as pending
	TArray<FString> UpdateHistoryPaths;
	for (auto& PendingItem : PendingItems) { UpdateHistoryPaths.Emplace(PendingItem->GetFilename()); }

	TSharedRef<FUpdateStatus, ESPMode::ThreadSafe> UpdateStatusOperation = ISourceControlOperation::Create<FUpdateStatus>();
	UpdateStatusOperation->SetUpdateHistory(true);
	ISourceControlModule::Get().GetProvider().Execute(UpdateStatusOperation, UpdateHistoryPaths, EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateSP(this, &STeamCityFileSelectWidget::OnUpdatedSourceControlHistory));
}

void STeamCityFileSelectWidget::OnUpdatedSourceControlHistory(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	FString ScanPath = FPaths::ProjectDir();
	if (ScanPath.EndsWith(TEXT("/"))) { ScanPath = ScanPath.Left(ScanPath.Len() - 1); } // trim the last slash

	FilePtr RootDirectoryPtr = FilePtr(new FFileItem(ScanPath));

	TArray<FilePtr> DirectoryItems; // note this is just an acceleration structure for identifying items we've already added to the tree, it doesn't get stored anywhere
	DirectoryItems.Add(RootDirectoryPtr);

	const TArray<FSourceControlStateRef> PendingItems(GetPendingSourceControlItems());
	for (auto& PendingItem : PendingItems)
	{
		EChangeType ChangeType = EChangeType::None;
		if (PendingItem->IsCheckedOut()) { ChangeType = EChangeType::Change; }
		if (PendingItem->IsAdded()) { ChangeType = EChangeType::Add; }
		if (PendingItem->IsDeleted()) { ChangeType = EChangeType::Remove; }

		AddItem(PendingItem->GetFilename(), DirectoryItems, ChangeType);
	}

	if (Items.Num() > 0) { CopyCheckState(RootDirectoryPtr, Items[0]); }

	Items = { RootDirectoryPtr };
	SetTreeItemExpansionRecursive(RootDirectoryPtr, true);
	FileSelectTreeView->RequestTreeRefresh();

	RefreshingFiles = false;
}

TArray<FSourceControlStateRef> STeamCityFileSelectWidget::GetPendingSourceControlItems() const
{
	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	return SourceControlProvider.GetCachedStateByPredicate(
		[](const FSourceControlStateRef& State)
		{
			return State->IsDeleted() || State->IsAdded() || State->IsCheckedOut();
		}
	);
}

bool STeamCityFileSelectWidget::CanRefresh() const
{
	return RefreshingFiles == false && ConnectedToSourceControl;
}

TArray<FFileItem> STeamCityFileSelectWidget::GetSelectedFiles(FFileItem* RootItem) const
{
	TArray<FFileItem> SelectedFiles;

	if(Items.Num() > 0)
	{
		if (RootItem == nullptr) { RootItem = Items[0].Get(); }

		for (auto& Child : RootItem->Children)
		{
			if (Child->Children.Num() > 0)
			{
				SelectedFiles.Append(GetSelectedFiles(Child.Get()));
			}
			else if (Child->GetCheckState() == ECheckBoxState::Checked)
			{
				SelectedFiles.Emplace(*Child.Get());
			}
		}
	}

	return SelectedFiles;
}
