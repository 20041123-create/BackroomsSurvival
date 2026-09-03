#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RandomRoomGenerationTypes.h"
#include "RandomRoomWorldGenerator.generated.h"

class ARandomRoomGateActor;
class ARandomRoomRuntimeActor;
class ARandomRoomSemanticAnchorActor;

/**
 * Server-authoritative runtime materializer for a deterministic plan. Clients receive the
 * summary and replicated room, gate, and anchor actors; they never re-run random generation.
 */
UCLASS(BlueprintType)
class RANDOMROOMGENERATIONRUNTIME_API ARandomRoomWorldGenerator : public AActor
{
	GENERATED_BODY()

public:
	ARandomRoomWorldGenerator();
	bool InitializeLayout(const FRandomRoomLayoutPlan& InPlan, float InGridCellSize);
	UFUNCTION(BlueprintCallable, Category="Random Room") bool AdvanceToPhase(int32 TargetPhaseIndex);
	UFUNCTION(BlueprintPure, Category="Random Room") int32 GetCurrentPhaseIndex() const { return CurrentPhaseIndex; }
	UFUNCTION(BlueprintPure, Category="Random Room") int32 GetMaterializedRoomCount() const { return MaterializedRoomCount; }
	UFUNCTION(BlueprintPure, Category="Random Room") int32 GetLayoutHash() const { return LayoutHash; }
	UFUNCTION(BlueprintCallable, Category="Random Room") ARandomRoomSemanticAnchorActor* SpawnSemanticAnchor(FGameplayTag AnchorTag, FRandomRoomHandle RoomHandle, FVector Location, FGameplayTagContainer ContextTags);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UFUNCTION() void OnRep_GenerationSummary();
	UFUNCTION(BlueprintImplementableEvent, Category="Random Room", meta=(DisplayName="On Room Materialized")) void ReceiveRoomMaterialized(ARandomRoomRuntimeActor* Room, FGameplayTag RoomType);
	virtual void ConfigureRoom(ARandomRoomRuntimeActor& Room, const FRandomRoomPlannedRoom& PlannedRoom);
	virtual void ConfigureRoomAnchors(ARandomRoomRuntimeActor& Room, const FRandomRoomPlannedRoom& PlannedRoom);
	bool MaterializeThroughPhase(int32 TargetPhaseIndex);
	bool SpawnPlannedRoom(const FRandomRoomPlannedRoom& PlannedRoom);
	void RefreshGates();
	void SpawnGate(const FRandomRoomPlannedRoom& Room, const FRandomRoomConnectorDefinition& Connector, bool bPermanentSeal);
	const FRandomRoomPlannedConnection* FindConnection(FRandomRoomHandle RoomHandle, FName ConnectorId) const;
	const FRandomRoomPlannedRoom* FindPlannedRoom(FRandomRoomHandle RoomHandle) const;
	FString MakeGateKey(FRandomRoomHandle RoomHandle, FName ConnectorId) const;
	FVector GetConnectorWorldLocation(const FRandomRoomPlannedRoom& Room, const FRandomRoomConnectorDefinition& Connector) const;
	FRotator GetConnectorWorldRotation(const FRandomRoomConnectorDefinition& Connector) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Random Room", meta=(ClampMin="100.0")) float GridCellSize = 2000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Random Room") TSubclassOf<ARandomRoomRuntimeActor> RoomActorClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Random Room") TSubclassOf<ARandomRoomGateActor> GateActorClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Random Room") TSubclassOf<ARandomRoomSemanticAnchorActor> AnchorActorClass;
	UPROPERTY(ReplicatedUsing=OnRep_GenerationSummary, VisibleAnywhere, BlueprintReadOnly, Category="Random Room|Replication") int32 AppliedSeed = 0;
	UPROPERTY(ReplicatedUsing=OnRep_GenerationSummary, VisibleAnywhere, BlueprintReadOnly, Category="Random Room|Replication") int32 LayoutHash = 0;
	UPROPERTY(ReplicatedUsing=OnRep_GenerationSummary, VisibleAnywhere, BlueprintReadOnly, Category="Random Room|Replication") int32 CurrentPhaseIndex = INDEX_NONE;
	UPROPERTY(ReplicatedUsing=OnRep_GenerationSummary, VisibleAnywhere, BlueprintReadOnly, Category="Random Room|Replication") int32 MaterializedRoomCount = 0;
	UPROPERTY(ReplicatedUsing=OnRep_GenerationSummary, VisibleAnywhere, BlueprintReadOnly, Category="Random Room|Replication") bool bGenerationSucceeded = false;
	FRandomRoomLayoutPlan LayoutPlan;
	TMap<int32, TObjectPtr<ARandomRoomRuntimeActor>> SpawnedRooms;
	TMap<FString, TWeakObjectPtr<ARandomRoomGateActor>> SpawnedGates;
};
