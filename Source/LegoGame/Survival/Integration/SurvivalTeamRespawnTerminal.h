#pragma once

#include "CoreMinimal.h"
#include "LegoGame/LegoGame.h"
#include "LegoGame/Survival/Contracts/SurvivalInterfaces.h"
#include "SurvivalTeamRespawnTerminal.generated.h"

class UStaticMeshComponent;
class USphereComponent;

/** Map-authored, team-owned interaction point for depositing respawn energy. */
UCLASS(Blueprintable)
class LEGOGAME_API ASurvivalTeamRespawnTerminal : public AActor, public ISurvivalInteractableInterface
{
	GENERATED_BODY()

public:
	ASurvivalTeamRespawnTerminal();
	virtual bool CanInteract_Implementation(APawn* InstigatorPawn) const override;
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;

	/** Server-authoritative terminal action; consumes the configured amount only on success. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Survival|Respawn")
	bool TryDepositRespawnEnergy(APawn* InstigatorPawn);

	ETeamType GetTeamType() const { return TeamType; }

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Survival|Respawn")
	TObjectPtr<UStaticMeshComponent> TerminalMesh;

	/** Explicit interaction range; terminal usability must not depend on the visual mesh collision. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Survival|Respawn")
	TObjectPtr<USphereComponent> InteractionBounds;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Survival|Respawn")
	ETeamType TeamType = ETeamType::ETT_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Survival|Respawn", meta=(ClampMin="1"))
	int32 DepositQuantity = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Survival|Respawn", meta=(ClampMin="1.0"))
	float InteractionDistance = 250.0f;
};
