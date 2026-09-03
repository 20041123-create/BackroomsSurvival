// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LegoGame/Subsystem/PropsSubsystem.h"
#include "LegoGame/Survival/Contracts/SurvivalTypes.h"
#include "PackageComponent.generated.h"


class AWeaponBase;
enum class ESkinType : uint8;
class ASceneItemActor;

USTRUCT()
struct FPackageItemNetEntry
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Key = INDEX_NONE;

	UPROPERTY()
	int32 ID = INDEX_NONE;
};

USTRUCT()
struct FEquippedSkinNetEntry
{
	GENERATED_BODY()

	UPROPERTY()
	ESkinType SkinType = ESkinType::EST_None;

	UPROPERTY()
	int32 ID = INDEX_NONE;
};

//创建代理(多播代理）
DECLARE_MULTICAST_DELEGATE_OneParam(NearItemActorChanged,ASceneItemActor*);

DECLARE_MULTICAST_DELEGATE_TwoParams(PackageItemChanged,int32,int32);

DECLARE_MULTICAST_DELEGATE_TwoParams(SkinChanged,ESkinType,int32);

DECLARE_MULTICAST_DELEGATE_OneParam(WeaponChanged,int32);
DECLARE_MULTICAST_DELEGATE_OneParam(SurvivalInventoryChanged, const TArray<FItemStack>&);
DECLARE_MULTICAST_DELEGATE_OneParam(SurvivalInventorySelectionChanged, int32);


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LEGOGAME_API UPackageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UPackageComponent();
	//创建代理对象
	NearItemActorChanged OnAddNearItemActor;
	NearItemActorChanged OnRemoveNearItemActor;
	
	PackageItemChanged OnAddItemToPackage;
	PackageItemChanged OnRemoveItemFromPackage;
	
	SkinChanged OnPutOnSkin;
	SkinChanged OnTakeOffSkin;
	
	WeaponChanged OnEquipWeapon;
	WeaponChanged OnUnEquipWeapon;
	SurvivalInventoryChanged OnSurvivalInventoryChanged;
	SurvivalInventorySelectionChanged OnSurvivalInventorySelectionChanged;
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	void AddItemToPackage(int32 ID);
	
	int32 AllowPackageKey()const;
	
	bool RemoveItemFromPackage(int32 Key,int32& ID);
	
	bool PutOnSkin(int32 ID,ESkinType SkinType);
	
	int32 TakeOffSkin(ESkinType SkinType);
	
	UFUNCTION(BlueprintCallable)
	void EquipWeapon(int32 ID);
	bool TryEquipWeapon(int32 ID);
	bool TryAddLegacyPackageItem(int32 ID);
	
	UFUNCTION()
	void OnRep_HoldWeapon();

	UFUNCTION()
	void OnRep_PackageSnapshot();

	UFUNCTION()
	void OnRep_SkinSnapshot();

	UFUNCTION()
	void OnRep_SurvivalItemStacks();

	void RebuildPackageSnapshot();
	void RebuildSkinSnapshot();
	int32 FindLowestAvailableSurvivalSlot(const TArray<FItemStack>& Items) const;
	bool IsSurvivalResourceItem(int32 ItemId) const;
	bool IsSurvivalMirrorPackageKey(int32 PackageKey) const;
	int32 GetSurvivalMirrorPackageKey(int32 SlotId) const;
	void SynchronizeSurvivalPackageMap();
	void NotifySurvivalInventoryChanged();
	
	//RPC
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_PickItemFromNear(ASceneItemActor* SceneItemActor);
	UFUNCTION(Client, Reliable)
	void Client_OnAddItemToPackage(int32 Key, int32 ID);
	UFUNCTION(Server, Reliable,WithValidation)
	void Server_RemoveItemFromPackageToScene(int32 Key);
	UFUNCTION(Client, Reliable)
	void Client_OnRemoveItemFromPackage(int32 Key, int32 ID);
	
	UFUNCTION(Server, Reliable,WithValidation)
	void Server_PutOnSkinFromNear(ASceneItemActor* SceneItemActor,ESkinType SkinType = ESkinType::EST_None);
	UFUNCTION(Server, Reliable,WithValidation)
	void Server_PutOnSkinFromPackage(int32 Key,ESkinType SkinType);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_TakeOffToPackage(ESkinType SkinType);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_TakeOffToScene(ESkinType SkinType);
	
	UFUNCTION(NetMulticast, Reliable)
	void Multi_OnPutOnSkin(ESkinType SkinType, int32 ID);
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_EquipWeaponFromNear(ASceneItemActor* SceneItemActor);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_EquipWeaponFromPackage(int32 Key);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_UnEquipWeaponToScene();
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_UnEquipWeaponToPackage();
	
	UFUNCTION(NetMulticast, Reliable)
	void Multi_OnEquipWeapon(int32 ID);

	UFUNCTION(Server, Reliable)
	void Server_RequestTransferItemStack(int32 SlotId, int32 Quantity, AActor* Destination);

	UFUNCTION(Server, Reliable)
	void Server_RequestDropItemStack(int32 SlotId, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void Server_RequestConsumeItemStack(int32 SlotId, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void Server_RequestInteract(AActor* Target);
	
public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	
	
	//当玩家附近有道具进入
	void AddNearSceneItem(TObjectPtr<ASceneItemActor> SceneItem);
	void RemoveNearSceneItem(TObjectPtr<ASceneItemActor> SceneItem);
	
	TArray<ASceneItemActor*> GetNearItems() const { return NearSceneItems; }
	const TMap<int32,int32>& GetPackageItems() const { return PackageMap; }
	void BroadcastCurrentEquipmentState();
	const TArray<FItemStack>& GetSurvivalItemStacks() const { return SurvivalItemStacks; }
	void GetSurvivalInventoryItems(TArray<FSurvivalItemView>& OutItems) const;
	bool GetSurvivalInventoryItem(int32 SlotId, FSurvivalItemView& OutItem) const;
	bool GetSurvivalStackForPackageKey(int32 PackageKey, FItemStack& OutStack) const;
	bool TryAddItemStack(const FItemStack& ItemStack);
	bool TryRemoveItemStack(int32 SlotId, int32 Quantity, FItemStack& RemovedStack);
	/** Returns the total of valid positive stacks whose item tags match ItemTag. */
	int32 GetItemQuantityByTag(FGameplayTag ItemTag) const;
	/** Server-only all-or-nothing tagged consumption with a single inventory update. */
	bool TryConsumeItemsByTag(FGameplayTag ItemTag, int32 Quantity);
	bool TryTransferItemStack(int32 SlotId, int32 Quantity, AActor* Destination);
	void TransferAllSurvivalItemsTo(AActor* Destination);
	UFUNCTION(BlueprintCallable, Category="Survival|Inventory")
	void RequestTransferItemStack(int32 SlotId, int32 Quantity, AActor* Destination);
	UFUNCTION(BlueprintCallable, Category="Survival|Inventory")
	void RequestDropItemStack(int32 SlotId, int32 Quantity);
	UFUNCTION(BlueprintCallable, Category="Survival|Inventory")
	void RequestConsumeItemStack(int32 SlotId, int32 Quantity);
	UFUNCTION(BlueprintCallable, Category="Survival|Inventory")
	void SetSelectedSurvivalSlotId(int32 SlotId);
	UFUNCTION(BlueprintPure, Category="Survival|Inventory")
	int32 GetSelectedSurvivalSlotId() const { return SelectedSurvivalSlotId; }
	UFUNCTION(BlueprintCallable, Category="Survival|Interaction")
	void RequestInteract(AActor* Target);
	bool ConsumeItemById(int32 ItemId, int32 Quantity);
	int32 GetItemQuantityById(int32 ItemId) const;
	bool TryCraftRecipe(const FSurvivalRecipeDefinition& Recipe, int32 CraftCount);
	void DropAllSurvivalItems();
	
	//拾取道具进入背包中
	void PickItemFromNear(ASceneItemActor* Actor);
	//移除一个背包元素到场景中
	void RemoveItemFromPackageToScene(int32 Key);
	
	ASceneItemActor* SpawnSceneItemActorFromPlayerNear(int32 ID);
	ASceneItemActor* SpawnSceneItemActorFromPlayerNear(const FItemStack& ItemStack);
	
	//穿戴道具
	void PutOnSkinFromNear(ASceneItemActor* SceneItemActor,ESkinType SkinType = ESkinType::EST_None);
	void PutOnSkinFromPackage(int32 Key,ESkinType SkinType);
	
	//从穿戴列表脱下
	void TakeOffToPackage(ESkinType SkinType);
	void TakeOffToScene(ESkinType SkinType);
	
	//装备武器
	void EquipWeaponFromNear(ASceneItemActor* SceneItemActor);
	void EquipWeaponFromPackage(int32 Key);
	
	//卸载武器
	void UnEquipWeaponToScene();
	void UnEquipWeaponToPackage();
	void EquipWeaponFromPackage_Internal(int32 Key);
	
	TObjectPtr<AWeaponBase> GetHoldWeapon() const {return HoldWeapon;}
	
protected:
	
	UPROPERTY()
	TArray<ASceneItemActor*> NearSceneItems;
	
	TMap<int32,int32> PackageMap;
	
	TMap<ESkinType,int32> SkinMap;
	
	UPROPERTY(ReplicatedUsing=OnRep_HoldWeapon)
	TObjectPtr<AWeaponBase> HoldWeapon;

	UPROPERTY(ReplicatedUsing=OnRep_PackageSnapshot)
	TArray<FPackageItemNetEntry> PackageSnapshot;

	UPROPERTY(ReplicatedUsing=OnRep_SkinSnapshot)
	TArray<FEquippedSkinNetEntry> SkinSnapshot;

	UPROPERTY(ReplicatedUsing=OnRep_SurvivalItemStacks)
	TArray<FItemStack> SurvivalItemStacks;

	UPROPERTY(EditDefaultsOnly, Category="Survival|Inventory", meta=(ClampMin="1"))
	int32 SurvivalMaxSlots = 24;

	// UI-local selection only. Server inventory requests still validate their SlotId.
	int32 SelectedSurvivalSlotId = INDEX_NONE;

	// Legacy package slots occupy the low range. Survival stack mirrors are UI-only
	// compatibility entries and must never be used as an authoritative inventory.
	static constexpr int32 SurvivalMirrorPackageKeyBase = 1000000;
	
};



