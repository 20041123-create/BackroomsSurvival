#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LegoGame/Survival/Contracts/SurvivalTypes.h"
#include "SurvivalSemanticAnchorActor.generated.h"

class USceneComponent;

UCLASS(BlueprintType)
class LEGOGAME_API ASurvivalSemanticAnchorActor : public AActor
{
	GENERATED_BODY()

public:
	ASurvivalSemanticAnchorActor();

	void InitializeAnchor(FGameplayTag InAnchorTag, FRoomHandle InRoomHandle, ETeamType InTeamType = ETeamType::ETT_None);

	UFUNCTION(BlueprintPure, Category="Survival|Anchor")
	FGameplayTag GetAnchorTag() const { return AnchorTag; }

	UFUNCTION(BlueprintPure, Category="Survival|Anchor")
	FRoomHandle GetOwningRoomHandle() const { return OwningRoomHandle; }

	UFUNCTION(BlueprintPure, Category="Survival|Anchor")
	ETeamType GetTeamType() const { return TeamType; }

	UFUNCTION(BlueprintPure, Category="Survival|Anchor")
	bool IsAnchorEnabled() const { return bAnchorEnabled; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Survival|Anchor")
	void SetAnchorEnabled(bool bEnabled);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_AnchorDefinition();

	void ApplyActorTag();

	UPROPERTY(VisibleAnywhere, Category="Survival|Anchor")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(ReplicatedUsing=OnRep_AnchorDefinition, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Anchor")
	FGameplayTag AnchorTag;

	UPROPERTY(ReplicatedUsing=OnRep_AnchorDefinition, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Anchor")
	FRoomHandle OwningRoomHandle;

	UPROPERTY(ReplicatedUsing=OnRep_AnchorDefinition, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Anchor")
	ETeamType TeamType = ETeamType::ETT_None;

	UPROPERTY(ReplicatedUsing=OnRep_AnchorDefinition, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Anchor")
	bool bAnchorEnabled = true;
};
