#pragma once
#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"

enum class EChangeType : uint8
{
	None,
	Add,
	Remove,
	Change
};

struct FFileItem : public TSharedFromThis<FFileItem>
{
	FFileItem(const FString InPath)
		: Path(FPaths::ConvertRelativePathToFull(InPath))
		, CheckState(ECheckBoxState::Unchecked)
		, ChangeType(EChangeType::None)
	{}

	FFileItem(const FString& InPath, TSharedPtr<FFileItem> InParent)
		: Path(FPaths::ConvertRelativePathToFull(InPath))
		, CheckState(ECheckBoxState::Unchecked)
		, ChangeType(EChangeType::None)
		, Parent(InParent)
	{}

	FFileItem(const FString& InPath, TSharedPtr<FFileItem> InParent, EChangeType InChangeType)
		: Path(FPaths::ConvertRelativePathToFull(InPath))
		, CheckState(ECheckBoxState::Unchecked)
		, ChangeType(InChangeType)
		, Parent(InParent)
	{}

	ECheckBoxState GetCheckState() const;
	void SetCheckState(ECheckBoxState State);

	void SetCheckState_RecursiveUp(ECheckBoxState State);
	void SetCheckState_RecursiveDown(ECheckBoxState State);

	const FSlateBrush* GetSCCStateImage() const;

	FString Path;
	ECheckBoxState CheckState;
	EChangeType ChangeType;

	TSharedPtr<FFileItem> Parent;
	TArray<TSharedPtr<FFileItem>> Children;
};

typedef TSharedPtr<FFileItem> FilePtr;
