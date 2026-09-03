#include "SurvivalEndingWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "LegoGame/Survival/Match/SurvivalGameState.h"
#include "LegoGame/Survival/Match/SurvivalPlayerState.h"

namespace
{
	constexpr TCHAR HallMapPath[] = TEXT("/Game/LegoGame/Maps/HallMap");
}

void USurvivalEndingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SetRenderOpacity(0.0f);

	if (Button_61)
	{
		Button_61->OnClicked.RemoveAll(this);
		Button_61->OnClicked.AddDynamic(this, &ThisClass::HandleReturnToHallClicked);
	}

	BindGameState();
	RefreshPresentation();
}

void USurvivalEndingWidget::NativeDestruct()
{
	if (Button_61)
	{
		Button_61->OnClicked.RemoveAll(this);
	}
	UnbindGameState();
	Super::NativeDestruct();
}

void USurvivalEndingWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (GetWorld() && GetWorld()->GetTimeSeconds() >= NextRefreshWorldTime)
	{
		NextRefreshWorldTime = GetWorld()->GetTimeSeconds() + 0.1f;
		BindGameState();
		RefreshPresentation();
	}
}

void USurvivalEndingWidget::BindGameState()
{
	ASurvivalGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ASurvivalGameState>() : nullptr;
	if (BoundGameState.Get() == GameState)
	{
		return;
	}

	UnbindGameState();
	BoundGameState = GameState;
	if (GameState)
	{
		GameState->OnSurvivalMatchStateChanged.AddUObject(this, &ThisClass::HandleMatchStateChanged);
	}
}

void USurvivalEndingWidget::UnbindGameState()
{
	if (ASurvivalGameState* GameState = BoundGameState.Get())
	{
		GameState->OnSurvivalMatchStateChanged.RemoveAll(this);
	}
	BoundGameState.Reset();
}

void USurvivalEndingWidget::HandleMatchStateChanged()
{
	RefreshPresentation();
}

void USurvivalEndingWidget::RefreshPresentation()
{
	BindGameState();
	const ASurvivalGameState* GameState = BoundGameState.Get();
	const APlayerController* PlayerController = GetOwningPlayer();
	const ASurvivalPlayerState* PlayerState = PlayerController
		? PlayerController->GetPlayerState<ASurvivalPlayerState>()
		: nullptr;
	if (!TextBlock_37 || !Button_61 || !GameState || !PlayerState
		|| GameState->GetSurvivalMatchPhase() != ESurvivalMatchPhase::PostMatch)
	{
		SetEndingVisible(false);
		return;
	}

	TextBlock_37->SetText(ResolveResultText(GameState->GetOutcome(), PlayerState->GetTeamType()));
	TextBlock_37->SetVisibility(ESlateVisibility::HitTestInvisible);
	Button_61->SetVisibility(ESlateVisibility::Visible);
	Button_61->SetIsEnabled(!bTravelRequested);
	SetEndingVisible(true);
}

void USurvivalEndingWidget::SetEndingVisible(bool bVisible)
{
	SetRenderOpacity(bVisible ? 1.0f : 0.0f);
	if (bEndingVisible == bVisible)
	{
		return;
	}

	bEndingVisible = bVisible;
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->bShowMouseCursor = bVisible;
		if (bVisible)
		{
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(TakeWidget());
			PlayerController->SetInputMode(InputMode);
		}
		else
		{
			PlayerController->SetInputMode(FInputModeGameOnly());
		}
	}
}

void USurvivalEndingWidget::HandleReturnToHallClicked()
{
	if (bTravelRequested)
	{
		return;
	}

	bTravelRequested = true;
	if (Button_61)
	{
		Button_61->SetIsEnabled(false);
	}

	APlayerController* PlayerController = GetOwningPlayer();
	UWorld* World = GetWorld();
	if (!PlayerController || !World)
	{
		return;
	}

	PlayerController->bShowMouseCursor = false;
	PlayerController->SetInputMode(FInputModeGameOnly());
	if (PlayerController->HasAuthority() && World->GetNetMode() != NM_Client)
	{
		World->ServerTravel(HallMapPath);
	}
	else
	{
		PlayerController->ClientTravel(HallMapPath, TRAVEL_Absolute);
	}
}

FText USurvivalEndingWidget::ResolveResultText(ESurvivalMatchOutcome Outcome, ETeamType LocalTeam)
{
	if (Outcome == ESurvivalMatchOutcome::Draw)
	{
		return NSLOCTEXT("Survival", "EndingDraw", "平局");
	}

	const ETeamType WinningTeam = Outcome == ESurvivalMatchOutcome::PoliceVictory
		? ETeamType::ETT_Police
		: (Outcome == ESurvivalMatchOutcome::BanditVictory ? ETeamType::ETT_Bandit : ETeamType::ETT_None);
	if (WinningTeam == ETeamType::ETT_None
		|| (LocalTeam != ETeamType::ETT_Police && LocalTeam != ETeamType::ETT_Bandit))
	{
		return NSLOCTEXT("Survival", "EndingComplete", "比赛结束");
	}

	return LocalTeam == WinningTeam
		? NSLOCTEXT("Survival", "EndingVictory", "获胜")
		: NSLOCTEXT("Survival", "EndingDefeat", "败北");
}
