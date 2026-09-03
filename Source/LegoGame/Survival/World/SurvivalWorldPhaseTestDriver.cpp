#include "SurvivalWorldPhaseTestDriver.h"

#include "EngineUtils.h"
#include "SurvivalWorldGenerator.h"
#include "TimerManager.h"

ASurvivalWorldPhaseTestDriver::ASurvivalWorldPhaseTestDriver()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASurvivalWorldPhaseTestDriver::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority() || !GetWorld() || GetWorld()->WorldType != EWorldType::PIE)
	{
		return;
	}
	for (TActorIterator<ASurvivalWorldGenerator> Iterator(GetWorld()); Iterator; ++Iterator)
	{
		Generator = *Iterator;
		break;
	}
	if (Generator.IsValid())
	{
		GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &ThisClass::AdvanceOnePhase, PhaseIntervalSeconds, true);
	}
}

void ASurvivalWorldPhaseTestDriver::AdvanceOnePhase()
{
	if (Generator.IsValid())
	{
		const FSurvivalWorldRuntimeSnapshot Snapshot =
			ISurvivalWorldRuntimeInterface::Execute_GetWorldRuntimeSnapshot(Generator.Get());
		ISurvivalWorldRuntimeInterface::Execute_RequestAdvanceToPhase(
			Generator.Get(),
			Snapshot.CurrentUnlockedPhaseIndex + 1);
	}
}
