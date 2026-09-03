#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LegoGame/Survival/Contracts/SurvivalTypes.h"
#include "SurvivalRoomGateActor.generated.h"

class UBoxComponent;
class UInstancedStaticMeshComponent;
class UStaticMesh;

UCLASS(BlueprintType)
class LEGOGAME_API ASurvivalRoomGateActor : public AActor
{
	GENERATED_BODY()

public:
	ASurvivalRoomGateActor();

	void InitializeGate(FRoomHandle InOwningRoom, bool bInPermanentSeal, float InOpeningWidth);
	void UnlockGate();

	UFUNCTION(BlueprintPure, Category="Survival|Gate")
	bool IsLocked() const { return bLocked; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_GateState();

	UFUNCTION()
	void OnRep_GateLayout();

	void ApplyGateState();
	void RebuildGateGeometry();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> GateMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> GateBlocker;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> WallMesh;

	UPROPERTY(ReplicatedUsing=OnRep_GateLayout, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Gate")
	float OpeningWidth = 1800.0f;

	UPROPERTY(ReplicatedUsing=OnRep_GateState, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Gate")
	bool bLocked = true;

	UPROPERTY(ReplicatedUsing=OnRep_GateState, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Gate")
	bool bPermanentSeal = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Gate")
	FRoomHandle OwningRoomHandle;
};
