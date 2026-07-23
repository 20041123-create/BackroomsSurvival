#include "HallUserInfoWidget.h"

#include "HallUserWidget.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "LegoGame/LegoGame.h"
#include "LegoGame/GamePlay/Hall/HallHUD.h"
#include "LegoGame/GamePlay/Hall/HallPlayerController.h"
#include "LegoGame/GamePlay/Hall/HallPlayerState.h"

void UHallUserInfoWidget::BindPlayerState(APlayerState* PlayerState)
{
	AHallPlayerState* HallPlayerState = Cast<AHallPlayerState>(PlayerState);
	if (!HallPlayerState)
	{
		return;
	}

	BindHallPlayerState = HallPlayerState;
	HallPlayerState->OnPlayerInfoChanged.AddUObject(this, &ThisClass::OnPlayerInfoChanged);
	HallPlayerState->OnReadyChanged.AddUObject(this, &ThisClass::OnPlayerReadyChanged);
	HallPlayerState->OnIsMaster.AddUObject(this, &ThisClass::OnMaster);

	if (AHallPlayerState* OwningPlayerState =
		GetOwningPlayer()->GetPlayerState<AHallPlayerState>())
	{
		OwningPlayerState->OnIsMaster.AddUObject(this, &ThisClass::RefreshKickAccess);
	}

	OnPlayerInfoChanged(
		HallPlayerState->GetHeadIndex(),
		HallPlayerState->GetHallPlayerName(),
		HallPlayerState->GetTeamType(),
		HallPlayerState->GetJobType());
	OnPlayerReadyChanged(HallPlayerState->IsReady());
	if (HallPlayerState->IsMaster())
	{
		OnMaster();
	}
	RefreshKickAccess();
}

void UHallUserInfoWidget::OnPlayerInfoChanged(
	int32 HeadIndex,
	const FString& PlayerName,
	ETeamType TeamType,
	EJobType JobType)
{
	OnHeadIndexChanged(HeadIndex);
	PlayerNameTextBlock->SetText(FText::FromString(PlayerName));
	JobTextBlock->SetText(LG::GetJobText(JobType));

	if (AHallHUD* HUD = Cast<AHallHUD>(GetOwningPlayer()->GetHUD()))
	{
		UScrollBox* ScrollBox = HUD->GetHallUserWidget()->GetScrollBox(TeamType);
		if (ScrollBox && ScrollBox != GetParent())
		{
			ScrollBox->AddChild(this);
		}
	}
}

void UHallUserInfoWidget::OnPlayerReadyChanged(bool bReady)
{
	BgBorder->SetBrushColor(
		bReady
			? FLinearColor(0.7f, 0.4f, 0.1f, 1.0f)
			: FLinearColor(0.0f, 0.3f, 0.3f, 1.0f));
}

void UHallUserInfoWidget::OnMaster()
{
	BgBorder->SetBrushColor(FLinearColor(0.3f, 1.0f, 0.1f, 1.0f));
	RefreshKickAccess();
}

void UHallUserInfoWidget::RefreshKickAccess()
{
	const AHallPlayerState* OwningPlayerState =
		GetOwningPlayer()->GetPlayerState<AHallPlayerState>();
	const bool bCanKick = OwningPlayerState && OwningPlayerState->IsMaster()
		&& BindHallPlayerState && !BindHallPlayerState->IsMaster();

	KickButton->SetVisibility(
		bCanKick ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (bCanKick)
	{
		KickButton->OnClicked.RemoveDynamic(this, &ThisClass::OnKickPlayer);
		KickButton->OnClicked.AddDynamic(this, &ThisClass::OnKickPlayer);
	}
}

void UHallUserInfoWidget::OnKickPlayer()
{
	if (AHallPlayerController* PlayerController =
		Cast<AHallPlayerController>(GetOwningPlayer()))
	{
		PlayerController->RequestKickPlayer(BindHallPlayerState);
	}
}
