#pragma once

#include "CoreMinimal.h"
#include "TeamCityEditor.h"
#include "TeamCityGetBuildConfigs.h"
#include "SlateFwd.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/SListView.h"
#include "Brushes/SlateColorBrush.h"

#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"

/** https://www.jetbrains.com/help/teamcity/build-state.html **/
UENUM()
enum class ETeamCityBuildState : uint8
{
	RunningSuccessfully,
	Successful,
	RunningAndFailing,
	Failed,
	Canceled
};

class FDisplayedTeamCityBuildInfo
{
public:
	FString DateTimeFinished;
	FString ProjectName;
	FString Name;
	FString Comment;
	const FSlateBrush* BuildStatusBrush;

	FSlateColorBrush BuildStatusColor = FSlateColorBrush(FLinearColor::Black);

	/** Static function for creating a new item, but ensures that you can only have a TSharedRef to one */
	static TSharedRef<FDisplayedTeamCityBuildInfo> Make(FString InDateTimeStarted, FString InProjectName, FString InBuildName, FString InComment, const FSlateBrush* InBuildStatusBrush)
	{
		return MakeShareable(new FDisplayedTeamCityBuildInfo(InDateTimeStarted, InProjectName, InBuildName, InComment, InBuildStatusBrush));
	}

	FSimpleDelegate Hyperlink;

protected:
	/** Hidden constructor, always use Make above */
	FDisplayedTeamCityBuildInfo(FString InDateTimeStarted, FString InProjectName, FString InBuildName, FString InComment, const FSlateBrush* InBuildStatusBrush)
		: DateTimeFinished(InDateTimeStarted)
		, ProjectName(InProjectName)
		, Name(InBuildName)
		, Comment(InComment)
		, BuildStatusBrush(InBuildStatusBrush)
	{}

	/** Hidden constructor, always use Make above */
	FDisplayedTeamCityBuildInfo() {}
};

typedef SListView< TSharedPtr<FDisplayedTeamCityBuildInfo> > STeamCityBuildListType;

//////////////////////////////////////////////////////////////////////////
// STeamCityBuildsViewer

class STeamCityBuildHistory : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STeamCityBuildHistory)
	{}
	
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~STeamCityBuildHistory();

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	TSharedRef<ITableRow> GenerateTeamCityBuildRow(TSharedPtr<FDisplayedTeamCityBuildInfo> InInfo, const TSharedRef<STableViewBase>& OwnerTable);

private:
	void OnGetBuildConfigs(BuildConfigPtr RootBuildConfig);
	void OnGetPersonalBuilds(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

	void OnConnectionFlushComplete(ETeamCityConnectionState TeamCityConnectionState);

	TSharedPtr<STeamCityBuildListType> TeamCityBuildsListView;
	TArray<TSharedPtr<FDisplayedTeamCityBuildInfo>> TeamCityBuildsList;

	TSharedPtr<FTeamCityGetBuildConfig> TeamCityGetBuildConfig;
	float TimeSinceLastProcess;
};
