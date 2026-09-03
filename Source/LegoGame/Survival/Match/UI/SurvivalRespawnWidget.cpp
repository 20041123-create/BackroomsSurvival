#include "SurvivalRespawnWidget.h"

#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "LegoGame/Survival/Match/SurvivalGameState.h"
#include "LegoGame/Survival/Match/SurvivalPlayerState.h"

void USurvivalRespawnWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// HUD creation can precede PlayerState replication in PIE. Keep Slate ticking
	// while transparent so the later replicated state can activate this overlay.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SetRenderOpacity(0.0f);
	BindPlayerState();
	RefreshPresentation();
}

void USurvivalRespawnWidget::NativeDestruct()
{
	UnbindPlayerState();
	Super::NativeDestruct();
}

void USurvivalRespawnWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (GetWorld() && GetWorld()->GetTimeSeconds() >= NextRefreshWorldTime)
	{
		NextRefreshWorldTime = GetWorld()->GetTimeSeconds() + 0.1f;
		// Seamless travel may replace the controller's PlayerState while the old
		// instance remains valid. Compare on every refresh so the overlay cannot
		// remain bound to stale replicated data.
		BindPlayerState();
		RefreshPresentation();
	}
}

void USurvivalRespawnWidget::BindPlayerState()
{
	ASurvivalPlayerState* PlayerState = nullptr;
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerState = PlayerController->GetPlayerState<ASurvivalPlayerState>();
	}
	if (BoundPlayerState.Get() == PlayerState)
	{
		return;
	}

	UnbindPlayerState();
	BoundPlayerState = PlayerState;
	if (PlayerState)
	{
		PlayerState->OnSurvivalLifeStateChanged.AddUObject(this, &ThisClass::HandlePlayerStateChanged);
	}
}

void USurvivalRespawnWidget::UnbindPlayerState()
{
	if (ASurvivalPlayerState* PlayerState = BoundPlayerState.Get())
	{
		PlayerState->OnSurvivalLifeStateChanged.RemoveAll(this);
	}
	BoundPlayerState.Reset();
}

void USurvivalRespawnWidget::HandlePlayerStateChanged()
{
	RefreshPresentation();
}

void USurvivalRespawnWidget::RefreshPresentation()
{
	BindPlayerState();

	if (!TextBlock_Num || !TextBlock_Show)
	{
		SetRenderOpacity(0.0f);
		return;
	}

	ASurvivalPlayerState* PlayerState = BoundPlayerState.Get();
	ASurvivalGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ASurvivalGameState>() : nullptr;
	if (!PlayerState || !GameState)
	{
		SetRenderOpacity(0.0f);
		return;
	}

	const FSurvivalRespawnPresentation Presentation = ResolvePresentation(
		PlayerState->GetSurvivalLifeState(), PlayerState->IsMatchParticipant(),
		PlayerState->GetRespawnQueuePosition(), PlayerState->GetRespawnReadyServerTime(),
		GameState->GetServerWorldTimeSeconds(), GameState->GetSurvivalMatchPhase());
	if (!Presentation.bVisible)
	{
		TextBlock_Num->SetText(FText::GetEmpty());
		TextBlock_Show->SetText(FText::GetEmpty());
		TextBlock_Num->SetVisibility(ESlateVisibility::Collapsed);
		TextBlock_Show->SetVisibility(ESlateVisibility::Collapsed);
		SetRenderOpacity(0.0f);
		return;
	}

	const bool bHasCountdown = Presentation.CountdownSeconds != INDEX_NONE;
	TextBlock_Num->SetText(bHasCountdown ? FText::AsNumber(Presentation.CountdownSeconds) : FText::GetEmpty());
	TextBlock_Num->SetVisibility(bHasCountdown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	TextBlock_Show->SetText(Presentation.StatusText);
	TextBlock_Show->SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(1.0f);
}

FSurvivalRespawnPresentation USurvivalRespawnWidget::ResolvePresentation(
	ESurvivalLifeState LifeState, bool bMatchParticipant, int32 QueuePosition,
	float RespawnReadyServerTime, float ServerTimeSeconds, ESurvivalMatchPhase MatchPhase)
{
	FSurvivalRespawnPresentation Result;
	if (MatchPhase == ESurvivalMatchPhase::PostMatch
		&& (LifeState == ESurvivalLifeState::Spectating || LifeState == ESurvivalLifeState::Eliminated))
	{
		Result.bVisible = true;
		Result.StatusText = NSLOCTEXT("Survival", "PostMatchSpectator", "比赛已结束：仅可观战");
		return Result;
	}

	const FText QueueText = QueuePosition > 0
		? FText::Format(NSLOCTEXT("Survival", "RespawnQueue", "复活队列 #{0}"), QueuePosition)
		: NSLOCTEXT("Survival", "JoiningRespawnQueue", "等待加入复活队列");

	switch (LifeState)
	{
	case ESurvivalLifeState::WaitingRespawn:
		Result.bVisible = true;
		if (RespawnReadyServerTime > 0.0f)
		{
			Result.CountdownSeconds = FMath::Max(0, FMath::CeilToInt(RespawnReadyServerTime - ServerTimeSeconds));
			Result.StatusText = QueueText;
		}
		else
		{
			Result.StatusText = FText::Format(
				NSLOCTEXT("Survival", "RespawnWaitingEnergy", "{0}\n等待复活能源"), QueueText);
		}
		break;
	case ESurvivalLifeState::Respawning:
		Result.bVisible = true;
		Result.CountdownSeconds = 0;
		Result.StatusText = NSLOCTEXT("Survival", "Respawning", "正在复活…");
		break;
	case ESurvivalLifeState::Spectating:
		if (!bMatchParticipant)
		{
			Result.bVisible = true;
			Result.StatusText = NSLOCTEXT("Survival", "LateJoinSpectator", "本局进行中加入：仅可观战");
		}
		break;
	case ESurvivalLifeState::Eliminated:
		Result.bVisible = true;
		Result.StatusText = NSLOCTEXT("Survival", "EliminatedSpectator", "已淘汰：仅可观战");
		break;
	default:
		break;
	}
	return Result;
}
