#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RandomRoomGenerationTypes.h"
#include "RandomRoomGateActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/** Replicated blocker for an unmaterialized connection or a permanently sealed connector. */
UCLASS(BlueprintType)
class RANDOMROOMGENERATIONRUNTIME_API ARandomRoomGateActor : public AActor
{
	GENERATED_BODY()

public:
	ARandomRoomGateActor();
	void InitializeGate(FRandomRoomHandle InOwningRoom, bool bInPermanentSeal);
	void UnlockGate();
	UFUNCTION(BlueprintPure, Category="Random Room") bool IsLocked() const { return bLocked; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UFUNCTION() void OnRep_GateState();
	void ApplyGateState();
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> GateMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> GateBlocker;
	UPROPERTY(ReplicatedUsing=OnRep_GateState, VisibleAnywhere, BlueprintReadOnly, Category="Random Room") bool bLocked = true;
	UPROPERTY(ReplicatedUsing=OnRep_GateState, VisibleAnywhere, BlueprintReadOnly, Category="Random Room") bool bPermanentSeal = false;
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Random Room") FRandomRoomHandle OwningRoomHandle;
};
