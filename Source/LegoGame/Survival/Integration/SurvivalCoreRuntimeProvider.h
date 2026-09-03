#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LegoGame/Survival/Contracts/SurvivalInterfaces.h"
#include "SurvivalCoreRuntimeProvider.generated.h"

class AEnemyCharacter;
class ASceneItemActor;

/**
 * Authority-world implementation of the Match-to-Core runtime spawn boundary.
 * Maps and GameModes discover this actor only through ISurvivalRuntimeSpawnInterface.
 */
UCLASS(Blueprintable)
class LEGOGAME_API ASurvivalCoreRuntimeProvider : public AActor, public ISurvivalRuntimeSpawnInterface
{
	GENERATED_BODY()

public:
	ASurvivalCoreRuntimeProvider();

	virtual FSurvivalRuntimeSpawnResult TrySpawnResource_Implementation(const FSurvivalResourceSpawnRequest& Request) override;
	virtual FSurvivalRuntimeSpawnResult TrySpawnEnemy_Implementation(const FSurvivalEnemySpawnRequest& Request) override;

	/** Class used for real pickup Actors. It must preserve ASceneItemActor interaction behavior. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Survival|Runtime Spawn|Resources")
	TSubclassOf<ASceneItemActor> ResourceActorClass;

	/** Core-private default used when the request has no EnemyArchetypeTag. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Survival|Runtime Spawn|Enemies")
	TSubclassOf<AEnemyCharacter> DefaultEnemyClass;

	/** Optional semantic tag returned for a default enemy request. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Survival|Runtime Spawn|Enemies")
	FGameplayTag DefaultEnemyArchetypeTag;

	/** Explicit Core-private enemy archetype mapping. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Survival|Runtime Spawn|Enemies")
	TMap<FGameplayTag, TSubclassOf<AEnemyCharacter>> EnemyArchetypeClasses;

	/** Exact-position policy used for both runtime Actor types. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Survival|Runtime Spawn")
	ESpawnActorCollisionHandlingMethod SpawnCollisionHandling = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

private:
	static FSurvivalRuntimeSpawnResult MakeFailure(ESurvivalRuntimeSpawnResultCode ResultCode, const FString& FailureReason,
		FGameplayTag ResolvedGameplayTag = FGameplayTag());
	static FSurvivalRuntimeSpawnResult MakeSuccess(AActor* SpawnedActor, FGameplayTag ResolvedGameplayTag);
	bool ResolveResourceItem(FGameplayTag ItemTag, int32& OutItemId, FGameplayTag& OutResolvedGameplayTag) const;
	bool ResolveEnemyClass(FGameplayTag RequestedArchetypeTag, TSubclassOf<AEnemyCharacter>& OutEnemyClass,
		FGameplayTag& OutResolvedGameplayTag) const;
	ESurvivalRuntimeSpawnResultCode GetSpawnFailureCode() const;
};
