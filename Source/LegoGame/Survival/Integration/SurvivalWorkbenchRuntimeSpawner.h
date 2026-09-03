#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LegoGame/Survival/Contracts/SurvivalTypes.h"
#include "SurvivalWorkbenchRuntimeSpawner.generated.h"

class ASurvivalWorkbenchActor;
struct FSurvivalWorkbenchRuntimeSpawnerTestAccess;

/**
 * Authority-only map adapter that materializes the configured workbench class
 * at enabled Anchor.Workbench locations after the World runtime has completed
 * its layout. It observes the public World runtime contract only; it never
 * reads World-private room or anchor actor types.
 */
UCLASS(BlueprintType)
class LEGOGAME_API ASurvivalWorkbenchRuntimeSpawner : public AActor
{
	GENERATED_BODY()

public:
	ASurvivalWorkbenchRuntimeSpawner();

	virtual void BeginPlay() override;

	/** Production Blueprint to materialize at each enabled workbench anchor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival|Workbench")
	TSubclassOf<ASurvivalWorkbenchActor> WorkbenchActorClass;

	/**
	 * Contract observation interval. This is not a resource refresh timer: a
	 * successful layout is synchronized once, and later only when its public
	 * signature changes (for example after a World phase unlock).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival|Workbench", meta = (ClampMin = "0.05"))
	float RefreshIntervalSeconds = 0.25f;

private:
	friend struct FSurvivalWorkbenchRuntimeSpawnerTestAccess;

	void RefreshWorkbenches();
	bool ResolveWorldRuntimeProvider();
	bool HasRuntimeSignatureChanged(const FSurvivalWorldRuntimeSnapshot& Snapshot) const;
	bool HasMissingWorkbenchForEnabledAnchor(const TArray<FSurvivalAnchorView>& Anchors) const;
	void SynchronizeWorkbenches(const FSurvivalWorldRuntimeSnapshot& Snapshot, const TArray<FSurvivalAnchorView>& Anchors);
	bool TrySpawnWorkbench(const FSurvivalAnchorView& Anchor);
	bool IsUsableWorkbenchAnchor(const FSurvivalAnchorView& Anchor) const;
	void RememberRuntimeSignature(const FSurvivalWorldRuntimeSnapshot& Snapshot);

	TWeakObjectPtr<AActor> WorldRuntimeProvider;
	TMap<int32, TWeakObjectPtr<ASurvivalWorkbenchActor>> SpawnedWorkbenchesByRoom;
	TSet<int32> LoggedDuplicateRoomHandles;

	bool bHasObservedSucceededLayout = false;
	bool bLoggedProviderError = false;
	bool bLoggedClassError = false;
	bool bLoggedLayoutFailure = false;
	int32 LastLayoutHash = 0;
	int32 LastUnlockedPhaseIndex = INDEX_NONE;
	int32 LastMaterializedRoomCount = 0;
	FTimerHandle RefreshTimerHandle;
};
