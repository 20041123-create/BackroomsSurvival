#include "SurvivalWorkbenchActor.h"

#include "LegoGame/Character/LgCharacterBase.h"
#include "LegoGame/Components/PackageComponent.h"

ASurvivalWorkbenchActor::ASurvivalWorkbenchActor()
{
	bReplicates = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

bool ASurvivalWorkbenchActor::CanInteract_Implementation(APawn* InstigatorPawn) const
{
	return IsValid(InstigatorPawn)
		&& FVector::DistSquared(InstigatorPawn->GetActorLocation(), GetActorLocation()) <= FMath::Square(250.0f);
}

void ASurvivalWorkbenchActor::Interact_Implementation(APawn* InstigatorPawn)
{
	if (!HasAuthority() || !CanInteract_Implementation(InstigatorPawn))
	{
		return;
	}

	if (ALgCharacterBase* Character = Cast<ALgCharacterBase>(InstigatorPawn))
	{
		Character->SetActiveSurvivalWorkbench(this);
	}
}

void ASurvivalWorkbenchActor::GetAvailableRecipes(TArray<FSurvivalRecipeDefinition>& OutRecipes) const
{
	OutRecipes = Recipes;
}

bool ASurvivalWorkbenchActor::TryCraft(APawn* CraftingPawn, FName RecipeId, int32 CraftCount)
{
	if (!HasAuthority() || !CanInteract_Implementation(CraftingPawn) || RecipeId.IsNone() || CraftCount <= 0)
	{
		return false;
	}

	const FSurvivalRecipeDefinition* Recipe = Recipes.FindByPredicate([RecipeId](const FSurvivalRecipeDefinition& Candidate)
	{
		return Candidate.RecipeId == RecipeId;
	});
	if (!Recipe)
	{
		return false;
	}
	if (Recipe->RequiredStationTag.IsValid() && !StationTag.MatchesTag(Recipe->RequiredStationTag))
	{
		return false;
	}

	if (ALgCharacterBase* Character = Cast<ALgCharacterBase>(CraftingPawn))
	{
		if (UPackageComponent* Package = Character->GetPackageComponent())
		{
			return Package->TryCraftRecipe(*Recipe, CraftCount);
		}
	}

	return false;
}
