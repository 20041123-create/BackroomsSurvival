#pragma once

#include "CoreMinimal.h"
#include "SurvivalMatchTypes.generated.h"

/** The authoritative outcome shown after a Survival match reaches PostMatch. */
UENUM(BlueprintType)
enum class ESurvivalMatchOutcome : uint8
{
	None,
	PoliceVictory,
	BanditVictory,
	Draw
};

/** Small replicated snapshot for the two server-side encounter directors. */
USTRUCT(BlueprintType)
struct LEGOGAME_API FSurvivalDirectorSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Survival|Director")
	int32 ResourceBudgetRemaining = 0;

	UPROPERTY(BlueprintReadOnly, Category="Survival|Director")
	int32 EnemyBudgetRemaining = 0;

	UPROPERTY(BlueprintReadOnly, Category="Survival|Director")
	int32 AliveEnemies = 0;
};
