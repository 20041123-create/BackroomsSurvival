#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RandomRoomGenerationTypes.h"
#include "RandomRoomSemanticAnchorActor.generated.h"

/** Project-neutral replicated semantic location. Context tags replace project-specific team or gameplay enums. */
UCLASS(BlueprintType)
class RANDOMROOMGENERATIONRUNTIME_API ARandomRoomSemanticAnchorActor : public AActor
{
	GENERATED_BODY()

public:
	ARandomRoomSemanticAnchorActor();
	void InitializeAnchor(FGameplayTag InAnchorTag, FRandomRoomHandle InRoomHandle, const FGameplayTagContainer& InContextTags = FGameplayTagContainer());
	UFUNCTION(BlueprintPure, Category="Random Room") FGameplayTag GetAnchorTag() const { return AnchorTag; }
	UFUNCTION(BlueprintPure, Category="Random Room") FRandomRoomHandle GetOwningRoomHandle() const { return OwningRoomHandle; }
	UFUNCTION(BlueprintPure, Category="Random Room") FGameplayTagContainer GetContextTags() const { return ContextTags; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UFUNCTION() void OnRep_AnchorDefinition();
	void ApplyActorTag();
	UPROPERTY(ReplicatedUsing=OnRep_AnchorDefinition, VisibleAnywhere, BlueprintReadOnly, Category="Random Room") FGameplayTag AnchorTag;
	UPROPERTY(ReplicatedUsing=OnRep_AnchorDefinition, VisibleAnywhere, BlueprintReadOnly, Category="Random Room") FRandomRoomHandle OwningRoomHandle;
	UPROPERTY(ReplicatedUsing=OnRep_AnchorDefinition, VisibleAnywhere, BlueprintReadOnly, Category="Random Room") FGameplayTagContainer ContextTags;
};
