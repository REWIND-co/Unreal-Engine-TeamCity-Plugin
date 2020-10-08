#include "TeamCityFileItem.h"

ECheckBoxState FFileItem::GetCheckState() const
{
	return CheckState;
}

void FFileItem::SetCheckState(ECheckBoxState State)
{
	SetCheckState_RecursiveUp(State);
	SetCheckState_RecursiveDown(State);
}

void FFileItem::SetCheckState_RecursiveUp(ECheckBoxState State)
{
	CheckState = State;

	// update the parent check state
	if (Parent.IsValid())
	{
		bool AllChildrenMatch = true;
		for (auto& Child : Parent->Children)
		{
			if (Child.Get() != this && Child->GetCheckState() != State) { AllChildrenMatch = false; break; }
		}

		ECheckBoxState ParentState = ECheckBoxState::Undetermined;
		if (AllChildrenMatch) { ParentState = State; }
		Parent->SetCheckState_RecursiveUp(ParentState);
	}
}

void FFileItem::SetCheckState_RecursiveDown(ECheckBoxState State)
{
	CheckState = State;

	// update children state
	for (auto& Child : Children) { Child->SetCheckState_RecursiveDown(CheckState); }
}

const FSlateBrush* FFileItem::GetSCCStateImage() const
{
	FName ImageName;
	switch (ChangeType)
	{
	case EChangeType::Add:
	{
		ImageName = "Perforce.OpenForAdd";
		break;
	}
	case EChangeType::Remove:
	{
		ImageName = "Perforce.MarkedForDelete";
		break;
	}
	case EChangeType::Change:
	{
		ImageName = "Perforce.CheckedOut";
		break;
	}
	}
	return FEditorStyle::GetBrush(ImageName);
}