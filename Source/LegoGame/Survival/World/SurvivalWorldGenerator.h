#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LegoGame/Survival/Contracts/SurvivalDataAssets.h"
#include "LegoGame/Survival/Contracts/SurvivalInterfaces.h"
#include "SurvivalLayoutPlanner.h"
#include "SurvivalWorldGenerator.generated.h"

class ASurvivalRoomGateActor;
class ASurvivalRoomRuntimeActor;
class ASurvivalSemanticAnchorActor;
struct FSurvivalWorldRuntimeTestAccess;

/** Server-authoritative coordinator for the generated Survival world. */
UCLASS(BlueprintType)
class LEGOGAME_API ASurvivalWorldGenerator : public AActor, public ISurvivalWorldRuntimeInterface
{
	GENERATED_BODY()

	friend struct FSurvivalWorldRuntimeTestAccess;

public:
	ASurvivalWorldGenerator();

	virtual void BeginPlay() override;

	virtual bool RequestGenerateInitialLayout_Implementation(USurvivalModeConfig* Config) override;
	virtual bool RequestAdvanceToPhase_Implementation(int32 TargetPhaseIndex) override;
	virtual FSurvivalWorldRuntimeSnapshot GetWorldRuntimeSnapshot_Implementation() const override;
	virtual void GetAnchorsByTag_Implementation(FGameplayTag AnchorTag, TArray<FSurvivalAnchorView>& OutAnchors) const override;
	virtual bool GetTeamPlayerStartTransform_Implementation(ETeamType TeamType, FTransform& OutTransform) const override;

	/** Legacy World entry point retained for existing maps and PIE tooling. */
	UFUNCTION(BlueprintCallable, Category="Survival|World")
	bool GenerateInitialPhase();

	/** Legacy World entry point retained for the editor PIE phase test driver. */
	UFUNCTION(BlueprintCallable, Category="Survival|World")
	bool AdvanceToPhase(int32 TargetPhaseIndex);

	UFUNCTION(BlueprintPure, Category="Survival|World")
	int32 GetCurrentPhaseIndex() const { return CurrentPhaseIndex; }

	UFUNCTION(BlueprintPure, Category="Survival|World")
	int32 GetMaterializedRoomCount() const { return MaterializedRoomCount; }

	UFUNCTION(BlueprintPure, Category="Survival|World")
	int32 GetLayoutHash() const { return LayoutHash; }

	UFUNCTION(BlueprintPure, Category="Survival|World")
	int32 GetAppliedSeed() const { return AppliedSeed; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_GenerationSummary();

	bool GenerateInitialLayout();
	bool MaterializeThroughPhase(int32 TargetPhaseIndex);
	void RollbackMaterializedRooms(const TArray<int32>& RoomHandleValues);
	bool IsValidTargetPhase(int32 TargetPhaseIndex) const;
	void SetLayoutFailure(const FString& FailureReason);
	bool SpawnPlannedRoom(const LG::Survival::World::FPlannedRoom& PlannedRoom);
	void SpawnAnchorsForRoom(ASurvivalRoomRuntimeActor& Room, const LG::Survival::World::FPlannedRoom& PlannedRoom);
	void SpawnAnchor(FGameplayTag AnchorTag, const FRoomHandle& RoomHandle, const FVector& Location, ETeamType TeamType = ETeamType::ETT_None);
	void RefreshGates();
	void RebuildRuntimeNavigation();
	void SpawnGate(const LG::Survival::World::FPlannedRoom& Room, const FRoomConnectorDefinition& Connector, bool bPermanentSeal);
	const LG::Survival::World::FPlannedConnection* FindConnection(const FRoomHandle& RoomHandle, const FName ConnectorId) const;
	const LG::Survival::World::FPlannedRoom* FindPlannedRoom(const FRoomHandle& RoomHandle) const;
	FString MakeGateKey(const FRoomHandle& RoomHandle, const FName ConnectorId) const;
	FVector GetConnectorWorldLocation(const LG::Survival::World::FPlannedRoom& Room, const FRoomConnectorDefinition& Connector) const;
	FRotator GetConnectorWorldRotation(const FRoomConnectorDefinition& Connector) const;
	FVector GetRoomCenter(const LG::Survival::World::FPlannedRoom& Room) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Survival|World")
	TObjectPtr<USurvivalModeConfig> ModeConfig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Survival|World")
	TObjectPtr<URoomTemplateData> StartRoomTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Survival|World")
	TArray<TObjectPtr<URoomTemplateData>> RoomTemplates;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Survival|World", meta=(ClampMin="100.0"))
	float GridCellSize = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Survival|World")
	bool bGenerateOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Survival|World")
	TSubclassOf<ASurvivalRoomRuntimeActor> FallbackRoomActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Survival|World")
	TSubclassOf<ASurvivalRoomGateActor> GateActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Survival|World")
	TSubclassOf<ASurvivalSemanticAnchorActor> AnchorActorClass;

	UPROPERTY(ReplicatedUsing=OnRep_GenerationSummary, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Replication")
	int32 AppliedSeed = 0;

	UPROPERTY(ReplicatedUsing=OnRep_GenerationSummary, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Replication")
	int32 LayoutHash = 0;

	UPROPERTY(ReplicatedUsing=OnRep_GenerationSummary, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Replication")
	int32 CurrentPhaseIndex = INDEX_NONE;

	UPROPERTY(ReplicatedUsing=OnRep_GenerationSummary, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Replication")
	int32 MaterializedRoomCount = 0;

	UPROPERTY(ReplicatedUsing=OnRep_GenerationSummary, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Replication")
	bool bGenerationSucceeded = false;

	UPROPERTY(ReplicatedUsing=OnRep_GenerationSummary, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Replication")
	ESurvivalWorldLayoutStatus LayoutStatus = ESurvivalWorldLayoutStatus::NotRequested;

	UPROPERTY(ReplicatedUsing=OnRep_GenerationSummary, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Replication")
	FString GenerationFailureReason;

	LG::Survival::World::FLayoutPlan LayoutPlan;
	TMap<int32, TObjectPtr<ASurvivalRoomRuntimeActor>> SpawnedRooms;
	TMap<FString, TWeakObjectPtr<ASurvivalRoomGateActor>> SpawnedGates;
};
