#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LegoGame/Survival/Contracts/SurvivalInterfaces.h"
#include "SurvivalWorkbenchActor.generated.h"

UCLASS(Blueprintable)
class LEGOGAME_API ASurvivalWorkbenchActor : public AActor, public ISurvivalInteractableInterface
{
	GENERATED_BODY()

public:
	ASurvivalWorkbenchActor();

	virtual bool CanInteract_Implementation(APawn* InstigatorPawn) const override;
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;

	void GetAvailableRecipes(TArray<FSurvivalRecipeDefinition>& OutRecipes) const;
	bool TryCraft(APawn* CraftingPawn, FName RecipeId, int32 CraftCount);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Survival|Crafting")
	FGameplayTag StationTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Survival|Crafting")
	TArray<FSurvivalRecipeDefinition> Recipes;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;
};
