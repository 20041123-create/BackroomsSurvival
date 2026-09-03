#include "SurvivalHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameFramework/PlayerController.h"
#include "Internationalization/Text.h"
#include "LegoGame/Survival/Contracts/SurvivalInterfaces.h"
#include "LegoGame/Survival/Match/SurvivalGameState.h"
#include "LegoGame/Survival/Match/SurvivalPlayerState.h"

void USurvivalHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildFallbackLayout();
	EnsureVitalsDisplay();
	BindSurvivalState();
	Refresh();
}

void USurvivalHUDWidget::NativeDestruct()
{
	UnbindSurvivalState();
	Super::NativeDestruct();
}

void USurvivalHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (GetWorld() && GetWorld()->GetTimeSeconds() >= NextRefreshWorldTime)
	{
		NextRefreshWorldTime = GetWorld()->GetTimeSeconds() + 0.25f;
		Refresh();
	}
}

void USurvivalHUDWidget::BuildFallbackLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SurvivalCanvas"));
	WidgetTree->RootWidget = Canvas;
	UVerticalBox* Panel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SurvivalPanel"));
	UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel);
	PanelSlot->SetPosition(FVector2D(32.0f, 32.0f));
	PanelSlot->SetAutoSize(true);

	const auto AddLine = [this, Panel](const FName Name)
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		TextBlock->SetAutoWrapText(true);
		TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Panel->AddChildToVerticalBox(TextBlock);
		return TextBlock;
	};

	PhaseText = AddLine(TEXT("PhaseText"));
	VitalsText = AddLine(TEXT("VitalsText"));
	TeamText = AddLine(TEXT("TeamText"));
	DirectorText = AddLine(TEXT("DirectorText"));
	OutcomeText = AddLine(TEXT("OutcomeText"));
}

void USurvivalHUDWidget::EnsureVitalsDisplay()
{
	if (VitalsText || !WidgetTree)
	{
		return;
	}

	VitalsText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("VitalsText")));
	if (VitalsText)
	{
		return;
	}

	UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Canvas)
	{
		UWidget* PreviousRoot = WidgetTree->RootWidget;
		Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SurvivalNativeOverlay"));
		WidgetTree->RootWidget = Canvas;
		if (PreviousRoot)
		{
			UCanvasPanelSlot* RootSlot = Canvas->AddChildToCanvas(PreviousRoot);
			RootSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			RootSlot->SetOffsets(FMargin(0.0f));
		}
	}

	VitalsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("VitalsText"));
	VitalsText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	UCanvasPanelSlot* VitalsSlot = Canvas->AddChildToCanvas(VitalsText);
	VitalsSlot->SetAnchors(FAnchors(1.0f, 0.0f));
	VitalsSlot->SetAlignment(FVector2D(1.0f, 0.0f));
	VitalsSlot->SetPosition(FVector2D(-32.0f, 32.0f));
	VitalsSlot->SetAutoSize(true);
}

void USurvivalHUDWidget::BindSurvivalState()
{
	UnbindSurvivalState();

	BoundGameState = GetWorld() ? GetWorld()->GetGameState<ASurvivalGameState>() : nullptr;
	if (ASurvivalGameState* SurvivalGameState = BoundGameState.Get())
	{
		SurvivalGameState->OnSurvivalMatchStateChanged.AddUObject(this, &ThisClass::HandleSurvivalStateChanged);
	}

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		BoundPlayerState = PlayerController->GetPlayerState<ASurvivalPlayerState>();
		if (ASurvivalPlayerState* PlayerState = BoundPlayerState.Get())
		{
			PlayerState->OnSurvivalLifeStateChanged.AddUObject(this, &ThisClass::HandlePlayerLifeStateChanged);
		}
	}
}

void USurvivalHUDWidget::UnbindSurvivalState()
{
	if (ASurvivalGameState* SurvivalGameState = BoundGameState.Get())
	{
		SurvivalGameState->OnSurvivalMatchStateChanged.RemoveAll(this);
	}
	if (ASurvivalPlayerState* PlayerState = BoundPlayerState.Get())
	{
		PlayerState->OnSurvivalLifeStateChanged.RemoveAll(this);
	}

	BoundGameState.Reset();
	BoundPlayerState.Reset();
}

void USurvivalHUDWidget::HandleSurvivalStateChanged()
{
	Refresh();
}

void USurvivalHUDWidget::HandlePlayerLifeStateChanged()
{
	Refresh();
}

void USurvivalHUDWidget::Refresh()
{
	if (VitalsText)
	{
		VitalsText->SetText(GetVitalsText());
	}
	ASurvivalGameState* SurvivalGameState = BoundGameState.Get();
	if (!SurvivalGameState)
	{
		BindSurvivalState();
		SurvivalGameState = BoundGameState.Get();
	}

	if (!SurvivalGameState)
	{
		return;
	}

	const float ServerTimeSeconds = SurvivalGameState->GetServerWorldTimeSeconds();
	if (PhaseText)
	{
		PhaseText->SetText(GetPhaseText());
	}

	const FTeamSurvivalState PoliceState = SurvivalGameState->GetTeamState(ETeamType::ETT_Police);
	const FTeamSurvivalState BanditState = SurvivalGameState->GetTeamState(ETeamType::ETT_Bandit);
	if (TeamText)
	{
		TeamText->SetText(FText::Format(
			NSLOCTEXT("Survival", "TeamStatus", "警察  存活:{0} 等待:{1} 能源:{2}\n匪徒  存活:{3} 等待:{4} 能源:{5}"),
			PoliceState.AlivePlayers,
			PoliceState.WaitingPlayers,
			PoliceState.RespawnEnergy,
			BanditState.AlivePlayers,
			BanditState.WaitingPlayers,
			BanditState.RespawnEnergy));
	}

	if (DirectorText)
	{
		const FSurvivalDirectorSnapshot DirectorSnapshot = SurvivalGameState->GetDirectorSnapshot();
		DirectorText->SetText(FText::Format(
			NSLOCTEXT("Survival", "DirectorStatus", "房间:{0} 资源预算:{1} 敌人预算:{2} 存活怪物:{3}"),
			SurvivalGameState->GetUnlockedRoomCount(),
			DirectorSnapshot.ResourceBudgetRemaining,
			DirectorSnapshot.EnemyBudgetRemaining,
			DirectorSnapshot.AliveEnemies));
	}

	ASurvivalPlayerState* PlayerState = BoundPlayerState.Get();
	if (RespawnText)
	{
		if (PlayerState && PlayerState->GetSurvivalLifeState() == ESurvivalLifeState::WaitingRespawn)
		{
			const float ReadyTime = PlayerState->GetRespawnReadyServerTime();
			const FText DelayText = ReadyTime > 0.0f
				? FText::AsNumber(FMath::CeilToInt(FMath::Max(0.0f, ReadyTime - ServerTimeSeconds)))
				: NSLOCTEXT("Survival", "WaitingEnergy", "等待能源");
			RespawnText->SetText(FText::Format(
				NSLOCTEXT("Survival", "RespawnStatus", "复活队列 #{0}  {1}"),
				PlayerState->GetRespawnQueuePosition(),
				DelayText));
		}
		else if (PlayerState && PlayerState->GetSurvivalLifeState() == ESurvivalLifeState::Spectating
			&& !PlayerState->IsMatchParticipant())
		{
			RespawnText->SetText(NSLOCTEXT("Survival", "LateJoinSpectator", "本局进行中加入：仅可观战"));
		}
		else
		{
			RespawnText->SetText(FText::GetEmpty());
		}
	}

	if (OutcomeText)
	{
		OutcomeText->SetText(GetOutcomeText());
	}
}

FText USurvivalHUDWidget::GetVitalsText() const
{
	const APlayerController* PlayerController = GetOwningPlayer();
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Pawn || !Pawn->GetClass()->ImplementsInterface(USurvivalVitalsInterface::StaticClass()))
	{
		return NSLOCTEXT("Survival", "VitalsUnavailable", "HP --");
	}

	const FSurvivalVitalsSnapshot Snapshot = ISurvivalVitalsInterface::Execute_GetSurvivalVitalsSnapshot(Pawn);
	return FText::Format(
		NSLOCTEXT("Survival", "VitalsStatus", "HP {0}/{1}  Hunger {2}/{3}  Thirst {4}/{5}"),
		FMath::CeilToInt(Snapshot.Health),
		FMath::CeilToInt(Snapshot.MaxHealth),
		FMath::CeilToInt(Snapshot.Hunger),
		FMath::CeilToInt(Snapshot.MaxHunger),
		FMath::CeilToInt(Snapshot.Thirst),
		FMath::CeilToInt(Snapshot.MaxThirst));
}

FText USurvivalHUDWidget::GetPhaseText() const
{
	const ASurvivalGameState* SurvivalGameState = BoundGameState.Get();
	if (!SurvivalGameState)
	{
		return FText::GetEmpty();
	}

	switch (SurvivalGameState->GetSurvivalMatchPhase())
	{
	case ESurvivalMatchPhase::WaitingForLayout:
		return SurvivalGameState->IsLayoutReady()
			? NSLOCTEXT("Survival", "WaitingTeams", "等待 Police 与 Bandit 阵营加入")
			: NSLOCTEXT("Survival", "WaitingLayout", "正在准备生存布局");

	case ESurvivalMatchPhase::Countdown:
		return FText::Format(
			NSLOCTEXT("Survival", "Countdown", "比赛将在 {0} 秒后开始"),
			FMath::CeilToInt(FMath::Max(0.0f,
				SurvivalGameState->GetCountdownEndServerTime() - SurvivalGameState->GetServerWorldTimeSeconds())));

	case ESurvivalMatchPhase::InProgress:
		return FText::Format(
			NSLOCTEXT("Survival", "InProgress", "生存阶段 {0}  已坚持 {1} 秒"),
			SurvivalGameState->GetActiveDifficultyPhaseIndex(),
			FMath::FloorToInt(FMath::Max(0.0f,
				SurvivalGameState->GetServerWorldTimeSeconds() - SurvivalGameState->GetSurvivalStartServerTime())));

	case ESurvivalMatchPhase::PostMatch:
		return NSLOCTEXT("Survival", "PostMatch", "比赛结算中");
	}

	return FText::GetEmpty();
}

FText USurvivalHUDWidget::GetOutcomeText() const
{
	const ASurvivalGameState* SurvivalGameState = BoundGameState.Get();
	if (!SurvivalGameState || SurvivalGameState->GetSurvivalMatchPhase() != ESurvivalMatchPhase::PostMatch)
	{
		return FText::GetEmpty();
	}

	switch (SurvivalGameState->GetOutcome())
	{
	case ESurvivalMatchOutcome::PoliceVictory:
		return NSLOCTEXT("Survival", "PoliceVictory", "Police 阵营获胜");
	case ESurvivalMatchOutcome::BanditVictory:
		return NSLOCTEXT("Survival", "BanditVictory", "Bandit 阵营获胜");
	case ESurvivalMatchOutcome::Draw:
		return NSLOCTEXT("Survival", "Draw", "双方阵营同时失去生存资格，平局");
	default:
		return FText::GetEmpty();
	}
}
