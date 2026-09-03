#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "SurvivalWorldPhaseTestDriver.generated.h"

class ASurvivalWorldGenerator;

/**
 * Editor-only PIE smoke driver for exercising the production World runtime
 * contract before Match owns phase scheduling.
 */
UCLASS()
class LEGOGAME_API ASurvivalWorldPhaseTestDriver : public AActor
{
	GENERATED_BODY()

public:
	ASurvivalWorldPhaseTestDriver();

	virtual void BeginPlay() override;

	protected:
	void AdvanceOnePhase();

	UPROPERTY(EditAnywhere, Category="Survival|Test Stub", meta=(ClampMin="1.0"))
	float PhaseIntervalSeconds = 10.0f;

	TWeakObjectPtr<ASurvivalWorldGenerator> Generator;
	FTimerHandle PhaseTimerHandle;
};
