// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LegoGame/Subsystem/PropsSubsystem.h"
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
	
	UFUNCTION()
	void OnRep_HoldWeapon();

	UFUNCTION()
	void OnRep_PackageSnapshot();

	UFUNCTION()
	void OnRep_SkinSnapshot();

	void RebuildPackageSnapshot();
	void RebuildSkinSnapshot();
	
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
	
	//拾取道具进入背包中
	void PickItemFromNear(ASceneItemActor* Actor);
	//移除一个背包元素到场景中
	void RemoveItemFromPackageToScene(int32 Key);
	
	void SpawnSceneItemActorFromPlayerNear(int32 ID);
	
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
	
};



