#include "SurvivalCoreRuntimeProvider.h"

#include "LegoGame/Enemy/EnemyCharacter.h"
#include "LegoGame/Scene/SceneItemActor.h"
#include "LegoGame/Subsystem/PropsSubsystem.h"

ASurvivalCoreRuntimeProvider::ASurvivalCoreRuntimeProvider()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);
	ResourceActorClass = ASceneItemActor::StaticClass();
}

FSurvivalRuntimeSpawnResult ASurvivalCoreRuntimeProvider::TrySpawnResource_Implementation(const FSurvivalResourceSpawnRequest& Request)
{
	if (!HasAuthority())
	{
		return MakeFailure(ESurvivalRuntimeSpawnResultCode::RejectedNotAuthority, TEXT("Runtime resource spawning requires server authority."));
	}
	if (!Request.ItemTag.IsValid() || Request.Quantity <= 0)
	{
		return MakeFailure(ESurvivalRuntimeSpawnResultCode::InvalidRequest, TEXT("Resource requests require a valid ItemTag and positive Quantity."));
	}

	int32 ItemId = INDEX_NONE;
	FGameplayTag ResolvedTag;
	if (!ResolveResourceItem(Request.ItemTag, ItemId, ResolvedTag))
	{
		return MakeFailure(ESurvivalRuntimeSpawnResultCode::NoMatchingDefinition, TEXT("No Core item definition matches the requested ItemTag."));
	}
	const UWorld* ConstWorld = GetWorld();
	const UGameInstance* GameInstance = ConstWorld ? ConstWorld->GetGameInstance() : nullptr;
	const UPropsSubsystem* Props = GameInstance ? GameInstance->GetSubsystem<UPropsSubsystem>() : nullptr;
	if (!Props)
	{
		return MakeFailure(ESurvivalRuntimeSpawnResultCode::NoMatchingDefinition, TEXT("Core props data is unavailable."), ResolvedTag);
	}
	if (Props->IsSurvivalWeaponItem(ItemId)
		&& (Request.Quantity != 1 || !Props->IsValidSurvivalWeaponDefinition(ItemId)))
	{
		return MakeFailure(ESurvivalRuntimeSpawnResultCode::InvalidRequest,
			TEXT("Survival weapon requests require one valid production weapon pickup."), ResolvedTag);
	}
	if (!ResourceActorClass)
	{
		return MakeFailure(ESurvivalRuntimeSpawnResultCode::NoMatchingDefinition, TEXT("No resource Actor class is configured."), ResolvedTag);
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return MakeFailure(ESurvivalRuntimeSpawnResultCode::SpawnFailed, TEXT("The runtime provider has no valid World."), ResolvedTag);
	}

	ASceneItemActor* ResourceActor = World->SpawnActorDeferred<ASceneItemActor>(ResourceActorClass, Request.SpawnTransform,
		nullptr, nullptr, SpawnCollisionHandling);
	if (!ResourceActor)
	{
		return MakeFailure(GetSpawnFailureCode(), TEXT("The requested resource transform is blocked or the Actor could not be created."), ResolvedTag);
	}

	FItemStack ItemStack;
	ItemStack.ItemId = ItemId;
	ItemStack.Quantity = Request.Quantity;
	ItemStack.SlotId = INDEX_NONE;
	ResourceActor->SetItemStack(ItemStack);
	ResourceActor->FinishSpawning(Request.SpawnTransform);

	const FItemStack& SpawnedStack = ResourceActor->GetItemStack();
	if (!IsValid(ResourceActor) || !ResourceActor->GetIsReplicated() || !SpawnedStack.IsValid()
		|| SpawnedStack.ItemId != ItemId || SpawnedStack.Quantity != Request.Quantity || SpawnedStack.SlotId != INDEX_NONE)
	{
		if (IsValid(ResourceActor))
		{
			ResourceActor->Destroy();
		}
		return MakeFailure(ESurvivalRuntimeSpawnResultCode::SpawnFailed, TEXT("The resource Actor did not complete its required pickup initialization."), ResolvedTag);
	}

	return MakeSuccess(ResourceActor, ResolvedTag);
}

FSurvivalRuntimeSpawnResult ASurvivalCoreRuntimeProvider::TrySpawnEnemy_Implementation(const FSurvivalEnemySpawnRequest& Request)
{
	if (!HasAuthority())
	{
		return MakeFailure(ESurvivalRuntimeSpawnResultCode::RejectedNotAuthority, TEXT("Runtime enemy spawning requires server authority."));
	}
	if (!FMath::IsFinite(Request.DifficultyMultiplier) || Request.DifficultyMultiplier <= 0.0f)
	{
		return MakeFailure(ESurvivalRuntimeSpawnResultCode::InvalidRequest, TEXT("Enemy requests require a finite, positive DifficultyMultiplier."));
	}

	TSubclassOf<AEnemyCharacter> EnemyClass;
	FGameplayTag ResolvedTag;
	if (!ResolveEnemyClass(Request.EnemyArchetypeTag, EnemyClass, ResolvedTag))
	{
		return MakeFailure(ESurvivalRuntimeSpawnResultCode::NoMatchingDefinition, TEXT("No Core enemy class matches the requested archetype."));
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return MakeFailure(ESurvivalRuntimeSpawnResultCode::SpawnFailed, TEXT("The runtime provider has no valid World."), ResolvedTag);
	}

	AEnemyCharacter* Enemy = World->SpawnActorDeferred<AEnemyCharacter>(EnemyClass, Request.SpawnTransform,
		nullptr, nullptr, SpawnCollisionHandling);
	if (!Enemy)
	{
		return MakeFailure(GetSpawnFailureCode(), TEXT("The requested enemy transform is blocked or the Actor could not be created."), ResolvedTag);
	}
	if (!Enemy->InitializeSurvivalDifficulty(Request.DifficultyMultiplier))
	{
		Enemy->Destroy();
		return MakeFailure(ESurvivalRuntimeSpawnResultCode::SpawnFailed, TEXT("The enemy rejected initial difficulty initialization."), ResolvedTag);
	}
	Enemy->FinishSpawning(Request.SpawnTransform);

	if (!IsValid(Enemy) || !Enemy->GetIsReplicated() || !Enemy->IsReplicatingMovement())
	{
		if (IsValid(Enemy))
		{
			Enemy->Destroy();
		}
		return MakeFailure(ESurvivalRuntimeSpawnResultCode::SpawnFailed, TEXT("The enemy did not complete required replication initialization."), ResolvedTag);
	}

	return MakeSuccess(Enemy, ResolvedTag);
}

FSurvivalRuntimeSpawnResult ASurvivalCoreRuntimeProvider::MakeFailure(ESurvivalRuntimeSpawnResultCode ResultCode,
	const FString& FailureReason, FGameplayTag ResolvedGameplayTag)
{
	FSurvivalRuntimeSpawnResult Result;
	Result.bSucceeded = false;
	Result.ResultCode = ResultCode;
	Result.SpawnedActor = nullptr;
	Result.ResolvedGameplayTag = ResolvedGameplayTag;
	Result.FailureReason = FailureReason;
	return Result;
}

FSurvivalRuntimeSpawnResult ASurvivalCoreRuntimeProvider::MakeSuccess(AActor* SpawnedActor, FGameplayTag ResolvedGameplayTag)
{
	FSurvivalRuntimeSpawnResult Result;
	Result.bSucceeded = true;
	Result.ResultCode = ESurvivalRuntimeSpawnResultCode::Succeeded;
	Result.SpawnedActor = SpawnedActor;
	Result.ResolvedGameplayTag = ResolvedGameplayTag;
	Result.FailureReason.Reset();
	return Result;
}

bool ASurvivalCoreRuntimeProvider::ResolveResourceItem(FGameplayTag ItemTag, int32& OutItemId,
	FGameplayTag& OutResolvedGameplayTag) const
{
	OutItemId = INDEX_NONE;
	OutResolvedGameplayTag = FGameplayTag();
	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UPropsSubsystem* Props = GameInstance ? GameInstance->GetSubsystem<UPropsSubsystem>() : nullptr;
	if (!Props)
	{
		return false;
	}

	TArray<int32> CandidateItemIds;
	Props->GetSurvivalItemIdsByTag(ItemTag, CandidateItemIds);
	if (CandidateItemIds.IsEmpty())
	{
		return false;
	}

	OutItemId = CandidateItemIds[0];
	FSurvivalItemView ItemView;
	if (!Props->GetSurvivalItemView(OutItemId, ItemView) || !ItemView.ItemTags.HasTag(ItemTag))
	{
		OutItemId = INDEX_NONE;
		return false;
	}
	TArray<FGameplayTag> MatchingItemTags;
	ItemView.ItemTags.GetGameplayTagArray(MatchingItemTags);
	MatchingItemTags.RemoveAll([ItemTag](const FGameplayTag CandidateTag)
	{
		return !CandidateTag.MatchesTag(ItemTag);
	});
	MatchingItemTags.Sort([](const FGameplayTag Left, const FGameplayTag Right)
	{
		return Left.GetTagName().LexicalLess(Right.GetTagName());
	});
	OutResolvedGameplayTag = MatchingItemTags.IsEmpty() ? ItemTag : MatchingItemTags[0];
	return true;
}

bool ASurvivalCoreRuntimeProvider::ResolveEnemyClass(FGameplayTag RequestedArchetypeTag,
	TSubclassOf<AEnemyCharacter>& OutEnemyClass, FGameplayTag& OutResolvedGameplayTag) const
{
	OutEnemyClass = nullptr;
	OutResolvedGameplayTag = FGameplayTag();
	if (RequestedArchetypeTag.IsValid())
	{
		const TSubclassOf<AEnemyCharacter>* FoundClass = EnemyArchetypeClasses.Find(RequestedArchetypeTag);
		if (!FoundClass || !*FoundClass)
		{
			return false;
		}
		OutEnemyClass = *FoundClass;
		OutResolvedGameplayTag = RequestedArchetypeTag;
		return true;
	}

	if (!DefaultEnemyClass)
	{
		return false;
	}
	OutEnemyClass = DefaultEnemyClass;
	OutResolvedGameplayTag = DefaultEnemyArchetypeTag;
	return true;
}

ESurvivalRuntimeSpawnResultCode ASurvivalCoreRuntimeProvider::GetSpawnFailureCode() const
{
	return SpawnCollisionHandling == ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding
		|| SpawnCollisionHandling == ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding
		? ESurvivalRuntimeSpawnResultCode::SpawnBlocked
		: ESurvivalRuntimeSpawnResultCode::SpawnFailed;
}
