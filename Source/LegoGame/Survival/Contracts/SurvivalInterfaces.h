#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SurvivalDataAssets.h"
#include "SurvivalTypes.h"
#include "SurvivalInterfaces.generated.h"

UINTERFACE(BlueprintType)
class LEGOGAME_API USurvivalInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class LEGOGAME_API ISurvivalInteractableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|Interaction")
	bool CanInteract(APawn* InstigatorPawn) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|Interaction")
	void Interact(APawn* InstigatorPawn);
};

UINTERFACE(BlueprintType)
class LEGOGAME_API USurvivalInventoryInterface : public UInterface
{
	GENERATED_BODY()
};

class LEGOGAME_API ISurvivalInventoryInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|Inventory")
	void GetInventoryItems(TArray<FSurvivalItemView>& OutItems) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|Inventory")
	bool GetInventoryItem(int32 SlotId, FSurvivalItemView& OutItem) const;

	/**
	 * Returns the sum of valid positive stacks whose FSurvivalItemView.ItemTags
	 * match ItemTag through standard GameplayTag hierarchy matching.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|Inventory")
	int32 GetItemQuantityByTag(FGameplayTag ItemTag) const;

	/**
	 * Server-authoritative all-or-nothing tagged consumption. Implementations
	 * must verify authority in C++, validate ItemTag and Quantity, verify the
	 * total first, consume in deterministic SlotId order, and commit one
	 * inventory notification/replication update only after full success.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, BlueprintNativeEvent, Category="Survival|Inventory")
	bool TryConsumeItemsByTag(FGameplayTag ItemTag, int32 Quantity);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|Inventory")
	bool TryAddItemStack(const FItemStack& ItemStack);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|Inventory")
	bool TryRemoveItemStack(int32 SlotId, int32 Quantity, FItemStack& RemovedStack);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|Inventory")
	void TransferAllItemsTo(AActor* Destination);

	// Request methods are commands. Implementations must execute them on the
	// server or forward them through an owning replicated actor/component.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|Inventory")
	void RequestTransferItemStack(int32 SlotId, int32 Quantity, AActor* Destination);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|Inventory")
	void RequestDropItemStack(int32 SlotId, int32 Quantity);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|Inventory")
	void RequestConsumeItemStack(int32 SlotId, int32 Quantity);
};

UINTERFACE(BlueprintType)
class LEGOGAME_API USurvivalRuntimeSpawnInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Match-to-Core boundary for creating one real replicated runtime Actor per
 * accepted request. Implementations resolve Core-private definitions and do
 * not own Match budgets, phase scheduling, room unlocks, or victory rules.
 */
class LEGOGAME_API ISurvivalRuntimeSpawnInterface
{
	GENERATED_BODY()

public:
	/** Server-authoritative request for one initialized, real resource Actor. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, BlueprintNativeEvent, Category="Survival|Runtime Spawn")
	FSurvivalRuntimeSpawnResult TrySpawnResource(const FSurvivalResourceSpawnRequest& Request);

	/** Server-authoritative request for one initialized, real enemy Actor. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, BlueprintNativeEvent, Category="Survival|Runtime Spawn")
	FSurvivalRuntimeSpawnResult TrySpawnEnemy(const FSurvivalEnemySpawnRequest& Request);
};

UINTERFACE(BlueprintType)
class LEGOGAME_API USurvivalCraftingInterface : public UInterface
{
	GENERATED_BODY()
};

class LEGOGAME_API ISurvivalCraftingInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|Crafting")
	void GetAvailableRecipes(TArray<FSurvivalRecipeDefinition>& OutRecipes) const;

	// Implementations must execute this request on the server or forward it
	// through an owning replicated actor/component.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|Crafting")
	void RequestCraftRecipe(FName RecipeId, int32 CraftCount);
};

UINTERFACE(BlueprintType)
class LEGOGAME_API USurvivalVitalsInterface : public UInterface
{
	GENERATED_BODY()
};

class LEGOGAME_API ISurvivalVitalsInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|Vitals")
	FSurvivalVitalsSnapshot GetSurvivalVitalsSnapshot() const;
};

UINTERFACE(BlueprintType)
class LEGOGAME_API USurvivalWeaponStateInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Read-only Core-to-presentation boundary for the locally owned character's
 * replicated clip and reserve ammunition state.
 */
class LEGOGAME_API ISurvivalWeaponStateInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|Weapon")
	FSurvivalWeaponAmmoSnapshot GetSurvivalWeaponAmmoSnapshot() const;
};

UINTERFACE(BlueprintType)
class LEGOGAME_API USurvivalMatchStateInterface : public UInterface
{
	GENERATED_BODY()
};

class LEGOGAME_API ISurvivalMatchStateInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|Match")
	USurvivalModeConfig* GetSurvivalConfig() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|Match")
	ESurvivalMatchPhase GetSurvivalMatchPhase() const;
};

UINTERFACE(BlueprintType)
class LEGOGAME_API USurvivalWorldRuntimeInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Match-to-World boundary. Implementations own layout planning, room spawning,
 * gates, and anchors; callers only submit server-authoritative requests and
 * consume the public runtime snapshot and anchor views.
 */
class LEGOGAME_API ISurvivalWorldRuntimeInterface
{
	GENERATED_BODY()

public:
	/**
	 * Server-authoritative, one-shot initial layout request. Returns true only
	 * when the request is accepted. Callers must query GetWorldRuntimeSnapshot
	 * until it reaches Succeeded or Failed.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, BlueprintNativeEvent, Category="Survival|World")
	bool RequestGenerateInitialLayout(USurvivalModeConfig* Config);

	/**
	 * Server-authoritative monotonic phase request. Implementations must reject
	 * TargetPhaseIndex values that are not strictly greater than the current
	 * unlocked phase. World owns room materialization and gate state.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, BlueprintNativeEvent, Category="Survival|World")
	bool RequestAdvanceToPhase(int32 TargetPhaseIndex);

	/** Safe for listen and dedicated servers; does not expose World-private layout types. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|World")
	FSurvivalWorldRuntimeSnapshot GetWorldRuntimeSnapshot() const;

	/**
	 * Safe for listen and dedicated servers. Implementations reset OutAnchors
	 * and return all matching anchors, including disabled anchors.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|World")
	void GetAnchorsByTag(FGameplayTag AnchorTag, TArray<FSurvivalAnchorView>& OutAnchors) const;

	/**
	 * Safe for listen and dedicated servers. Returns an enabled, team-matching
	 * Anchor.PlayerStart transform for Police or Bandit only. Returns false and
	 * leaves OutTransform unchanged when no valid anchor exists.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|World")
	bool GetTeamPlayerStartTransform(ETeamType TeamType, FTransform& OutTransform) const;
};

UINTERFACE(BlueprintType)
class LEGOGAME_API USurvivalRoomRuntimeInterface : public UInterface
{
	GENERATED_BODY()
};

class LEGOGAME_API ISurvivalRoomRuntimeInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|Room")
	FRoomHandle GetRoomHandle() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|Room")
	FGameplayTag GetRoomTypeTag() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|Room")
	void SetRoomState(ESurvivalRoomState NewState);
};

UINTERFACE(BlueprintType)
class LEGOGAME_API USurvivalDeathListenerInterface : public UInterface
{
	GENERATED_BODY()
};

class LEGOGAME_API ISurvivalDeathListenerInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Survival|Death")
	void HandleSurvivalDeath(AActor* Victim, AController* InstigatorController, AActor* DamageCauser);
};
