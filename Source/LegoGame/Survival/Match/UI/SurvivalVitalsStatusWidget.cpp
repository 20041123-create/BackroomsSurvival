#include "SurvivalVitalsStatusWidget.h"

#include "Components/Image.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "LegoGame/Survival/Contracts/SurvivalInterfaces.h"
#include "LegoGame/Survival/Contracts/SurvivalTypes.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	const FName ProgressBarParameterName(TEXT("ProgressBar"));
}

void USurvivalVitalsStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	PowerProgressMaterial = PowerProgress ? PowerProgress->GetDynamicMaterial() : nullptr;
	SANProgressMaterial = SANProgress ? SANProgress->GetDynamicMaterial() : nullptr;
	if (!PowerProgressMaterial || !SANProgressMaterial)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Survival player-vitals HUD is missing a dynamic ProgressBar material (Power=%s, SAN=%s)."),
			PowerProgressMaterial ? TEXT("valid") : TEXT("missing"),
			SANProgressMaterial ? TEXT("valid") : TEXT("missing"));
	}
	// HUDs are constructed before possession in PIE. Keep this widget visible to
	// Slate (but fully transparent) so NativeTick can detect the later Pawn.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SetRenderOpacity(0.0f);
	RefreshVitals();
}

void USurvivalVitalsStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (GetWorld() && GetWorld()->GetTimeSeconds() >= NextRefreshWorldTime)
	{
		NextRefreshWorldTime = GetWorld()->GetTimeSeconds() + 0.1f;
		RefreshVitals();
	}
}

void USurvivalVitalsStatusWidget::RefreshVitals()
{
	if (!PowerProgressMaterial || !SANProgressMaterial)
	{
		SetRenderOpacity(0.0f);
		return;
	}

	UWorld* World = GetWorld();
	AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	APawn* Pawn = GetOwningPlayerPawn();
	if (!GameState || !GameState->GetClass()->ImplementsInterface(USurvivalMatchStateInterface::StaticClass())
		|| !Pawn || !Pawn->GetClass()->ImplementsInterface(USurvivalVitalsInterface::StaticClass()))
	{
		SetRenderOpacity(0.0f);
		return;
	}

	const FSurvivalVitalsSnapshot Snapshot = ISurvivalVitalsInterface::Execute_GetSurvivalVitalsSnapshot(Pawn);
	const ESurvivalMatchPhase SurvivalMatchPhase =
		ISurvivalMatchStateInterface::Execute_GetSurvivalMatchPhase(GameState);
	// PIE can enter UE's match InProgress state before the replicated Survival
	// phase catches up. Treat either authoritative start signal as gameplay.
	const ESurvivalMatchPhase DisplayPhase = ResolveDisplayPhase(SurvivalMatchPhase, GameState->HasMatchStarted());
	if (!ShouldDisplayVitals(
		DisplayPhase, true, Snapshot))
	{
		SetRenderOpacity(0.0f);
		return;
	}

	// Power is thirst; SAN is hunger. The material treats one as a full ring.
	PowerProgressMaterial->SetScalarParameterValue(
		ProgressBarParameterName, NormalizeVital(Snapshot.Thirst, Snapshot.MaxThirst));
	SANProgressMaterial->SetScalarParameterValue(
		ProgressBarParameterName, NormalizeVital(Snapshot.Hunger, Snapshot.MaxHunger));
	SetRenderOpacity(1.0f);
}

float USurvivalVitalsStatusWidget::NormalizeVital(float CurrentValue, float MaxValue)
{
	return MaxValue > KINDA_SMALL_NUMBER
		? FMath::Clamp(CurrentValue / MaxValue, 0.0f, 1.0f)
		: 0.0f;
}

ESurvivalMatchPhase USurvivalVitalsStatusWidget::ResolveDisplayPhase(ESurvivalMatchPhase SurvivalMatchPhase,
	bool bEngineMatchStarted)
{
	return SurvivalMatchPhase == ESurvivalMatchPhase::InProgress || bEngineMatchStarted
		? ESurvivalMatchPhase::InProgress
		: SurvivalMatchPhase;
}

bool USurvivalVitalsStatusWidget::ShouldDisplayVitals(ESurvivalMatchPhase MatchPhase, bool bHasVitalsPawn,
	const FSurvivalVitalsSnapshot& Snapshot)
{
	return MatchPhase == ESurvivalMatchPhase::InProgress
		&& bHasVitalsPawn
		&& Snapshot.LifeState == ESurvivalLifeState::Alive;
}
