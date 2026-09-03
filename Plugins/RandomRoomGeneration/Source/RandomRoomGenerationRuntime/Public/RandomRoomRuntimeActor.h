#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RandomRoomGenerationTypes.h"
#include "RandomRoomRuntimeActor.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

/** Generic replicated graybox room. Projects can subclass it or replace it through the generator's RoomActorClass. */
UCLASS(BlueprintType)
class RANDOMROOMGENERATIONRUNTIME_API ARandomRoomRuntimeActor : public AActor
{
	GENERATED_BODY()

public:
	ARandomRoomRuntimeActor();
	void InitializeRoom(const FRandomRoomPlannedRoom& InPlan, float InCellSize);
	void SetRoomState(ERandomRoomState NewState);

	UFUNCTION(BlueprintPure, Category="Random Room") FRandomRoomHandle GetRoomHandle() const { return RoomHandle; }
	UFUNCTION(BlueprintPure, Category="Random Room") FGameplayTag GetRoomTypeTag() const { return RoomTypeTag; }
	UFUNCTION(BlueprintPure, Category="Random Room") int32 GetUnlockPhaseIndex() const { return UnlockPhaseIndex; }
	const TArray<FRandomRoomConnectorDefinition>& GetRuntimeConnectors() const { return RuntimeConnectors; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UFUNCTION() void OnRep_RoomLayout();
	UFUNCTION() void OnRep_RoomState();
	void RebuildGeometry();
	bool HasConnectorAt(const FIntPoint Cell, ERandomRoomConnectorDirection Direction) const;
	void AddWall(const FVector& RelativeLocation, const FVector& RelativeScale);

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(Transient) TObjectPtr<UStaticMesh> BlockMesh;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> GeneratedGeometry;
	UPROPERTY(ReplicatedUsing=OnRep_RoomLayout, VisibleAnywhere, BlueprintReadOnly, Category="Random Room") FRandomRoomHandle RoomHandle;
	UPROPERTY(ReplicatedUsing=OnRep_RoomLayout, VisibleAnywhere, BlueprintReadOnly, Category="Random Room") FName TemplateId = NAME_None;
	UPROPERTY(ReplicatedUsing=OnRep_RoomLayout, VisibleAnywhere, BlueprintReadOnly, Category="Random Room") FGameplayTag RoomTypeTag;
	UPROPERTY(ReplicatedUsing=OnRep_RoomLayout, VisibleAnywhere, BlueprintReadOnly, Category="Random Room") FIntPoint Footprint = FIntPoint(1, 1);
	UPROPERTY(ReplicatedUsing=OnRep_RoomLayout, VisibleAnywhere, BlueprintReadOnly, Category="Random Room") TArray<FRandomRoomConnectorDefinition> RuntimeConnectors;
	UPROPERTY(ReplicatedUsing=OnRep_RoomLayout, VisibleAnywhere, BlueprintReadOnly, Category="Random Room") int32 RotationQuarterTurns = 0;
	UPROPERTY(ReplicatedUsing=OnRep_RoomLayout, VisibleAnywhere, BlueprintReadOnly, Category="Random Room") int32 UnlockPhaseIndex = 0;
	UPROPERTY(ReplicatedUsing=OnRep_RoomLayout, VisibleAnywhere, BlueprintReadOnly, Category="Random Room") float CellSize = 2000.0f;
	UPROPERTY(ReplicatedUsing=OnRep_RoomState, VisibleAnywhere, BlueprintReadOnly, Category="Random Room") ERandomRoomState RoomState = ERandomRoomState::Planned;
};
