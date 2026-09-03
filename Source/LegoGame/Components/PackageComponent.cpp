// Fill out your copyright notice in the Description page of Project Settings.


#include "PackageComponent.h"

#include "LegoGame/LegoGame.h"
#include "LegoGame/Character/LgCharacterBase.h"
#include "LegoGame/Scene/SceneItemActor.h"
#include "LegoGame/Subsystem/PropsSubsystem.h"
#include "LegoGame/Survival/Contracts/SurvivalInterfaces.h"
#include "LegoGame/Survival/SurvivalVitalsComponent.h"
#include "LegoGame/Weapon/WeaponBase.h"
#include "Net/UnrealNetwork.h"


// Sets default values for this component's properties
UPackageComponent::UPackageComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	//开启网络同步
	//动态开启（需要在beginplay或之后开启）？时机过晚
	//SetIsReplicated(true);
	//构造函数中使用
	SetIsReplicatedByDefault(true);
	// ...
}


// Called when the game starts
void UPackageComponent::BeginPlay()
{
	Super::BeginPlay();
	

	// ...
	
}

void UPackageComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UPackageComponent, HoldWeapon);
	DOREPLIFETIME_CONDITION(UPackageComponent, PackageSnapshot, COND_OwnerOnly);
	DOREPLIFETIME(UPackageComponent, SkinSnapshot);
	DOREPLIFETIME_CONDITION(UPackageComponent, SurvivalItemStacks, COND_OwnerOnly);
}

void UPackageComponent::RebuildPackageSnapshot()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	PackageSnapshot.Reset(PackageMap.Num());
	TArray<int32> Keys;
	PackageMap.GetKeys(Keys);
	Keys.Sort();
	for (const int32 Key : Keys)
	{
		FPackageItemNetEntry& Entry = PackageSnapshot.AddDefaulted_GetRef();
		Entry.Key = Key;
		Entry.ID = PackageMap[Key];
	}
	GetOwner()->ForceNetUpdate();
}

void UPackageComponent::RebuildSkinSnapshot()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	SkinSnapshot.Reset(SkinMap.Num());
	TArray<ESkinType> SkinTypes;
	SkinMap.GetKeys(SkinTypes);
	SkinTypes.Sort([](const ESkinType Left, const ESkinType Right)
	{
		return static_cast<uint8>(Left) < static_cast<uint8>(Right);
	});
	for (const ESkinType SkinType : SkinTypes)
	{
		FEquippedSkinNetEntry& Entry = SkinSnapshot.AddDefaulted_GetRef();
		Entry.SkinType = SkinType;
		Entry.ID = SkinMap[SkinType];
	}
	GetOwner()->ForceNetUpdate();
}

void UPackageComponent::OnRep_PackageSnapshot()
{
	const TMap<int32, int32> PreviousItems = PackageMap;
	PackageMap.Reset();
	for (const FPackageItemNetEntry& Entry : PackageSnapshot)
	{
		if (Entry.Key >= 0 && Entry.ID >= 0)
		{
			PackageMap.Add(Entry.Key, Entry.ID);
		}
	}

	for (const TPair<int32, int32>& PreviousItem : PreviousItems)
	{
		if (!PackageMap.Contains(PreviousItem.Key)
			|| PackageMap[PreviousItem.Key] != PreviousItem.Value)
		{
			OnRemoveItemFromPackage.Broadcast(PreviousItem.Key, PreviousItem.Value);
		}
	}
	for (const TPair<int32, int32>& Item : PackageMap)
	{
		if (!PreviousItems.Contains(Item.Key) || PreviousItems[Item.Key] != Item.Value)
		{
			OnAddItemToPackage.Broadcast(Item.Key, Item.Value);
		}
	}
}

void UPackageComponent::OnRep_SkinSnapshot()
{
	const TMap<ESkinType, int32> PreviousSkins = SkinMap;
	SkinMap.Reset();
	for (const FEquippedSkinNetEntry& Entry : SkinSnapshot)
	{
		if (Entry.SkinType != ESkinType::EST_None && Entry.ID >= 0)
		{
			SkinMap.Add(Entry.SkinType, Entry.ID);
		}
	}

	for (const TPair<ESkinType, int32>& PreviousSkin : PreviousSkins)
	{
		if (!SkinMap.Contains(PreviousSkin.Key))
		{
			OnTakeOffSkin.Broadcast(PreviousSkin.Key, PreviousSkin.Value);
		}
	}
	for (const TPair<ESkinType, int32>& Skin : SkinMap)
	{
		if (!PreviousSkins.Contains(Skin.Key) || PreviousSkins[Skin.Key] != Skin.Value)
		{
			OnPutOnSkin.Broadcast(Skin.Key, Skin.Value);
		}
	}
}

void UPackageComponent::OnRep_SurvivalItemStacks()
{
	NotifySurvivalInventoryChanged();
}


void UPackageComponent::BroadcastCurrentEquipmentState()
{
	for (const TPair<ESkinType, int32>& Skin : SkinMap)
	{
		OnPutOnSkin.Broadcast(Skin.Key, Skin.Value);
	}
	if (HoldWeapon)
	{
		OnEquipWeapon.Broadcast(HoldWeapon->GetID());
	}
	else
	{
		OnUnEquipWeapon.Broadcast(INDEX_NONE);
	}
}

// Called every frame
void UPackageComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UPackageComponent::AddNearSceneItem(TObjectPtr<ASceneItemActor> SceneItem)
{
	//添加道具到容器中
	NearSceneItems.AddUnique(SceneItem);//AddUnique防止添加相同的
	//进行代理广播
	//检查是否存在有效绑定
	if (OnAddNearItemActor.IsBound())
	{
		//广播
		OnAddNearItemActor.Broadcast(SceneItem);
	}
	
}

void UPackageComponent::RemoveNearSceneItem(TObjectPtr<ASceneItemActor> SceneItem)
{
	NearSceneItems.Remove(SceneItem);
	if (OnRemoveNearItemActor.IsBound())
	{
		OnRemoveNearItemActor.Broadcast(SceneItem);
	}
	
}

void UPackageComponent::PickItemFromNear(ASceneItemActor* SceneItemActorActor)
{
	if (IsValid(SceneItemActorActor))
	{
		if (!GetOwner()->HasAuthority())
		{
			Server_PickItemFromNear(SceneItemActorActor);
			return;
		}
		if (FVector::DistSquared(SceneItemActorActor->GetActorLocation(), GetOwner()->GetActorLocation())
			> FMath::Square(250.0f))
		{
			return;
		}
		const FItemStack& SceneStack = SceneItemActorActor->GetItemStack();
		if (IsSurvivalResourceItem(SceneStack.ItemId))
		{
			if (TryAddItemStack(SceneStack))
			{
				SceneItemActorActor->Destroy();
			}
			return;
		}

		UPropsSubsystem* Props = GetWorld()->GetGameInstance()->GetSubsystem<UPropsSubsystem>();
		if (Props && Props->IsSurvivalWeaponItem(SceneStack.ItemId))
		{
			if (Props->IsValidSurvivalWeaponDefinition(SceneStack.ItemId)
				&& SceneStack.Quantity == 1 && TryAddLegacyPackageItem(SceneStack.ItemId))
			{
				SceneItemActorActor->Destroy();
			}
			return;
		}

		AddItemToPackage(SceneItemActorActor->GetID());
		//道具拾取完毕，移除地面上的Actor
		SceneItemActorActor->Destroy();
	}
	
}


void UPackageComponent::AddItemToPackage(int32 ID)
{
	//将Id记录到我们的Map容器中
	TryAddLegacyPackageItem(ID);
}

bool UPackageComponent::TryAddLegacyPackageItem(int32 ID)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || ID < 0)
	{
		return false;
	}

	const int32 Key = AllowPackageKey();
	PackageMap.Add(Key,ID);
	RebuildPackageSnapshot();
	Client_OnAddItemToPackage(Key,ID);
	return true;
}

void UPackageComponent::Client_OnAddItemToPackage_Implementation(int32 Key, int32 ID)
{
	bool bShouldNotify = GetOwner()->HasAuthority();
	//对于客户端来说没有背包数据
	if (!GetOwner()->HasAuthority())
	{
		if (!PackageMap.Contains(Key) || PackageMap[Key] != ID)
		{
			PackageMap.Add(Key,ID);
			bShouldNotify = true;
		}
	}
	
	if (bShouldNotify && OnAddItemToPackage.IsBound())
	{
		OnAddItemToPackage.Broadcast(Key,ID);
	}
	
}

//这个函数是从0开始正向检查哪个key没有被使用，则返回
int32 UPackageComponent::AllowPackageKey() const
{
	int32 Key = 0;
	while (PackageMap.Contains(Key))//检查背包map是否有当前的key数据，如果有，则表明被占用，则递增
	{
		Key++;
	}
	return Key;
}

bool UPackageComponent::RemoveItemFromPackage(int32 Key,int32& ID)
{
	if (PackageMap.Contains(Key))
	{
		ID = PackageMap[Key];
		PackageMap.Remove(Key);
		RebuildPackageSnapshot();
		Client_OnRemoveItemFromPackage(Key,ID);
		return true;
	}
	return false;
}

void UPackageComponent::Client_OnRemoveItemFromPackage_Implementation(int32 Key, int32 ID)
{
	//广播
	if (OnRemoveItemFromPackage.IsBound()
		&& (GetOwner()->HasAuthority() || PackageMap.Contains(Key)))
	{
		OnRemoveItemFromPackage.Broadcast(Key,ID);
	}
	if (!GetOwner()->HasAuthority())
	{
		if (PackageMap.Contains(Key))
		{
			PackageMap.Remove(Key);
		}
		
	}
}

void UPackageComponent::RemoveItemFromPackageToScene(int32 Key)
{
	if (!GetOwner()->HasAuthority())
	{
		if (PackageMap.Contains(Key))
		{
			Server_RemoveItemFromPackageToScene(Key);
		}
		return;
	}

	FItemStack SurvivalStack;
	if (GetSurvivalStackForPackageKey(Key, SurvivalStack))
	{
		RequestDropItemStack(SurvivalStack.SlotId, SurvivalStack.Quantity);
		return;
	}
	
	if (!PackageMap.Contains(Key) || IsSurvivalMirrorPackageKey(Key))
	{
		return;
	}
	if (!SpawnSceneItemActorFromPlayerNear(PackageMap[Key]))
	{
		return;
	}

	int32 ID = 0;
	if (RemoveItemFromPackage(Key,ID))//成功表明删除完毕
	{
		//从地面附近放置道具
		// The pickup was successfully spawned before removing its authoritative package entry.
	}
}

void UPackageComponent::Server_RemoveItemFromPackageToScene_Implementation(int32 Key)
{
	RemoveItemFromPackageToScene(Key);
	
}

bool UPackageComponent::Server_RemoveItemFromPackageToScene_Validate(int32 Key)
{
	return true;
}

ASceneItemActor* UPackageComponent::SpawnSceneItemActorFromPlayerNear(int32 ID)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || ID < 0)
	{
		return nullptr;
	}
	//从玩家附近生成一个道具
	//让一个向量绕着另外一个向量旋转一个角度
	FVector NewDirection = GetOwner()->GetActorForwardVector().RotateAngleAxis(FMath::FRandRange(-90.f,90.f),FVector::UpVector);//让使用组件的角色的正方向，绕着世界的上方向旋转-90，90
	FVector Location = GetOwner()->GetActorLocation() + NewDirection * FMath::FRandRange(100.f,150.f);
	
	//将物体放置到地面通过帧检查来完成
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit,Location,Location+FVector::UpVector*-500,ECC_Visibility))
	{
		Location = Hit.Location;//重新记录地面位置信息
	}
	
	
	
	//创建Actor（会显示不到附近Actor界面中）
	// ASceneItemActor* SceneItemActor = GetWorld()->SpawnActor<ASceneItemActor>(ASceneItemActor::StaticClass(),Location,FRotator::ZeroRotator);
	// //
	// if (SceneItemActor)
	// {
	// 	SceneItemActor->SetID(ID);
	// 	SceneItemActor->InitMesh();
	// }
	FTransform Transform;
	Transform.SetLocation(Location);
	//SpawnActorDeferred此函数是生成一个Actor但是不加到世界中，所有关于世界执行的逻辑不会执行
	ASceneItemActor* SceneItemActor = GetWorld()->SpawnActorDeferred<ASceneItemActor>(ASceneItemActor::StaticClass(),Transform);
	if (SceneItemActor)
	{
		SceneItemActor->SetID(ID);
		//告知引擎，Actor需要添加到世界，已经完成了生成操作
		SceneItemActor->FinishSpawning(Transform);//FinishSpawning此函数为了告知引擎可以添加Actor到世界中
	}
	return SceneItemActor;
}

bool UPackageComponent::PutOnSkin(int32 ID, ESkinType SkinType)
{
	//穿戴检查
	const FPropsBase* PropsBase = GetWorld()->GetGameInstance()->GetSubsystem<UPropsSubsystem>()->GetPropsById(ID);
	if (!PropsBase||PropsBase->Type == EPropsType::EPT_Weapon)
	{
		return false;
	}
	//检查穿戴位置是否正确
	const FSkinHeader* SkinHeader = static_cast<const FSkinHeader*>(PropsBase);
	if (SkinType==ESkinType::EST_None)//说明给AI穿戴
	{
		SkinType = SkinHeader->SkinType;
	}
	else if (SkinHeader->SkinType != SkinType)
	{
		return false;
	}
	//脱掉旧的穿戴新的
	if (SkinMap.Contains(SkinType))//说明穿戴位置上有数据
	{
		AddItemToPackage(SkinMap[SkinType]);
		SkinMap[SkinType] = ID;
	}
	else
	{
		SkinMap.Add(SkinType,ID);
	}
	RebuildSkinSnapshot();
	if (OnPutOnSkin.IsBound())
	{
		OnPutOnSkin.Broadcast(SkinType, ID);
	}
	
	return true;
}

void UPackageComponent::Multi_OnPutOnSkin_Implementation(ESkinType SkinType, int32 ID)
{
	if (OnPutOnSkin.IsBound())
	{
		OnPutOnSkin.Broadcast(SkinType,ID);
	}
	//更新数据
	if (!SkinMap.Contains(SkinType))
	{
		SkinMap.Add(SkinType,ID);
	}
}

int32 UPackageComponent::TakeOffSkin(ESkinType SkinType)
{
	if (SkinMap.Contains(SkinType))
	{
		const int32 ID = SkinMap[SkinType];
		SkinMap.Remove(SkinType);
		RebuildSkinSnapshot();
		
		//代理广播
		if (OnTakeOffSkin.IsBound())
		{
			OnTakeOffSkin.Broadcast(SkinType,ID);
		}
		return ID;
	}
	return -1;
}

void UPackageComponent::EquipWeapon(int32 ID)
{
	TryEquipWeapon(ID);
}

bool UPackageComponent::TryEquipWeapon(int32 ID)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	UPropsSubsystem* Props = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UPropsSubsystem>() : nullptr;
	if (!Props || !Props->IsValidSurvivalWeaponDefinition(ID))
	{
		return false;
	}

	const FWeaponBaseHeader* WeaponDefinition = static_cast<const FWeaponBaseHeader*>(Props->GetPropsById(ID));
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWeaponBase* NewWeapon = GetWorld()->SpawnActor<AWeaponBase>(WeaponDefinition->WeaponClass,
		GetOwner()->GetActorTransform(), SpawnParams);
	if (!NewWeapon)
	{
		return false;
	}

	if (HoldWeapon)
	{
		const int32 AmmoItemId = HoldWeapon->GetAmmoItemId();
		const int32 LoadedAmmo = HoldWeapon->GetCurrentClipVolume();
		if (AmmoItemId != INDEX_NONE && LoadedAmmo > 0)
		{
			FItemStack AmmoStack;
			AmmoStack.ItemId = AmmoItemId;
			AmmoStack.Quantity = LoadedAmmo;
			if (!TryAddItemStack(AmmoStack))
			{
				NewWeapon->Destroy();
				return false;
			}
			HoldWeapon->ExtractLoadedAmmo();
		}

		if (!TryAddLegacyPackageItem(HoldWeapon->GetID()))
		{
			NewWeapon->Destroy();
			return false;
		}
		HoldWeapon->Destroy();
	}

	HoldWeapon = NewWeapon;
	HoldWeapon->SetID(ID);
	HoldWeapon->SetMaster(Cast<ALgCharacterBase>(GetOwner()));
	HoldWeapon->PrepareForSurvivalInventory();
	OnEquipWeapon.Broadcast(ID);
	GetOwner()->ForceNetUpdate();
	return true;
}

void UPackageComponent::OnRep_HoldWeapon()
{
	// 当客户端收到服务端同步过来的 HoldWeapon 指针时，此函数会被调用
	if (HoldWeapon)
	{
		if (ALgCharacterBase* Character = Cast<ALgCharacterBase>(GetOwner()))
		{
			// 客户端本地也调用一次挂载逻辑
			HoldWeapon->SetMaster(Character);
		}
		// 【关键修复3：当且仅当武器确实存在时，才通知客户端 UI 和动画蓝图】
		if (OnEquipWeapon.IsBound())
		{
			OnEquipWeapon.Broadcast(HoldWeapon->GetID());
		}
	}
	else if (OnUnEquipWeapon.IsBound())
	{
		OnUnEquipWeapon.Broadcast(-1);
	}
	
}

void UPackageComponent::Multi_OnEquipWeapon_Implementation(int32 ID)
{
	if (OnEquipWeapon.IsBound())
	{
		OnEquipWeapon.Broadcast(ID);
	}
	
}

void UPackageComponent::Server_PickItemFromNear_Implementation(ASceneItemActor* SceneItemActor)
{
	PickItemFromNear(SceneItemActor);
	
}

bool UPackageComponent::Server_PickItemFromNear_Validate(ASceneItemActor* SceneItemActor)
{
	//安全校验
	return true;
	
}

void UPackageComponent::PutOnSkinFromNear(ASceneItemActor* SceneItemActor, ESkinType SkinType)
{
	if (!GetOwner()->HasAuthority())
	{
		if (IsValid(SceneItemActor))
		{
			Server_PutOnSkinFromNear(SceneItemActor,SkinType);
		}
		return;
	}
	
	//穿戴装备
	if (IsValid(SceneItemActor)
		&& FVector::DistSquared(SceneItemActor->GetActorLocation(), GetOwner()->GetActorLocation())
			<= FMath::Square(250.0f)
		&& PutOnSkin(SceneItemActor->GetID(),SkinType))
	{
		//移除场景道具
		SceneItemActor->Destroy();
	}
	
}

void UPackageComponent::Server_PutOnSkinFromNear_Implementation(ASceneItemActor* SceneItemActor, ESkinType SkinType)
{
	PutOnSkinFromNear(SceneItemActor,SkinType);
}

bool UPackageComponent::Server_PutOnSkinFromNear_Validate(ASceneItemActor* SceneItemActor, ESkinType SkinType)
{
	return true;
}

void UPackageComponent::PutOnSkinFromPackage(int32 Key, ESkinType SkinType)
{
	if (IsSurvivalMirrorPackageKey(Key))
	{
		return;
	}

	if (!GetOwner()->HasAuthority())
	{
		if (PackageMap.Contains(Key))
		{
			Server_PutOnSkinFromPackage(Key, SkinType);
		}
		return;
	}
	
	if (PackageMap.Contains(Key)&&PutOnSkin(PackageMap[Key],SkinType))
	{
		//从背包移除道具
		int32 ID = 0;
		RemoveItemFromPackage(Key,ID);
	}
	
}

void UPackageComponent::Server_PutOnSkinFromPackage_Implementation(int32 Key, ESkinType SkinType)
{
	PutOnSkinFromPackage(Key,SkinType);
}

bool UPackageComponent::Server_PutOnSkinFromPackage_Validate(int32 Key, ESkinType SkinType)
{
	return true;
}

void UPackageComponent::TakeOffToPackage(ESkinType SkinType)
{
	if (!GetOwner()->HasAuthority())
	{
		Server_TakeOffToPackage(SkinType);
		return;
	}
	//处理脱掉装饰性道具
	int32 ID = TakeOffSkin(SkinType);
	if (ID>=0)
	{
		AddItemToPackage(ID);
	}
	
}

void UPackageComponent::TakeOffToScene(ESkinType SkinType)
{
	if (!GetOwner()->HasAuthority())
	{
		Server_TakeOffToScene(SkinType);
		return;
	}
	int32 ID = TakeOffSkin(SkinType);
	if (ID>=0)
	{
		SpawnSceneItemActorFromPlayerNear(ID);
	}
	
}

void UPackageComponent::Server_TakeOffToPackage_Implementation(ESkinType SkinType)
{
	TakeOffToPackage(SkinType);
}

bool UPackageComponent::Server_TakeOffToPackage_Validate(ESkinType SkinType)
{
	return SkinType != ESkinType::EST_None;
}

void UPackageComponent::Server_TakeOffToScene_Implementation(ESkinType SkinType)
{
	TakeOffToScene(SkinType);
}

bool UPackageComponent::Server_TakeOffToScene_Validate(ESkinType SkinType)
{
	return SkinType != ESkinType::EST_None;
}

void UPackageComponent::EquipWeaponFromNear(ASceneItemActor* SceneItemActor)
{
	if (!GetOwner()->HasAuthority())
	{
		if (IsValid(SceneItemActor))
		{
			Server_EquipWeaponFromNear(SceneItemActor);
		}
		return;
	}
	
	const UPropsSubsystem* Props = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UPropsSubsystem>() : nullptr;
	if (IsValid(SceneItemActor) && Props && Props->IsValidSurvivalWeaponDefinition(SceneItemActor->GetID())
		&& FVector::DistSquared(SceneItemActor->GetActorLocation(), GetOwner()->GetActorLocation())
			<= FMath::Square(250.0f))
	{
		//装备武器
		if (SceneItemActor->GetItemStack().Quantity == 1 && TryEquipWeapon(SceneItemActor->GetID()))
		{
			SceneItemActor->Destroy();
		}
	}
	
}

void UPackageComponent::Server_EquipWeaponFromNear_Implementation(ASceneItemActor* SceneItemActor)
{
	EquipWeaponFromNear(SceneItemActor);
}

bool UPackageComponent::Server_EquipWeaponFromNear_Validate(ASceneItemActor* SceneItemActor)
{
	return true;
}


void UPackageComponent::EquipWeaponFromPackage(int32 Key)
{
	if (!IsValid(this) || !IsValid(GetOwner()))
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipWeaponFromPackage failed: Component or Owner is invalid!"));
		return;
	}
	if (IsSurvivalMirrorPackageKey(Key))
	{
		return;
	}
	
	if (!GetOwner()->HasAuthority())
	{
		if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
		{
			if (PackageMap.Contains(Key))
			{
				Server_EquipWeaponFromPackage(Key);
			}
			
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Client attempted to send Server RPC on a non-owned character!"));
		}
		return;
	}
	
	// if (PackageMap.Contains(Key)&&PackageMap[Key]>=WEAPON_INDEX)
	// {
	// 	EquipWeapon(PackageMap[Key]);
	// 	int32 ID = 0;
	// 	RemoveItemFromPackage(Key,ID);
	// }
	EquipWeaponFromPackage_Internal(Key);
	
}

void UPackageComponent::Server_EquipWeaponFromPackage_Implementation(int32 Key)
{
	//EquipWeaponFromPackage(Key);
	EquipWeaponFromPackage_Internal(Key);
}

bool UPackageComponent::Server_EquipWeaponFromPackage_Validate(int32 Key)
{
	return true;
}

void UPackageComponent::UnEquipWeaponToScene()
{
	if (!GetOwner()->HasAuthority())
	{
		Server_UnEquipWeaponToScene();
		return;
	}
	
	if (HoldWeapon)
	{
		const int32 AmmoItemId = HoldWeapon->GetAmmoItemId();
		const int32 LoadedAmmo = HoldWeapon->ExtractLoadedAmmo();
		if (AmmoItemId != INDEX_NONE && LoadedAmmo > 0)
		{
			FItemStack AmmoStack;
			AmmoStack.ItemId = AmmoItemId;
			AmmoStack.Quantity = LoadedAmmo;
			SpawnSceneItemActorFromPlayerNear(AmmoStack);
		}
		SpawnSceneItemActorFromPlayerNear(HoldWeapon->GetID());
		HoldWeapon->Destroy();
		HoldWeapon = nullptr;
		GetOwner()->ForceNetUpdate();
	}
	if (OnUnEquipWeapon.IsBound())
	{
		OnUnEquipWeapon.Broadcast(-1);
	}
}

void UPackageComponent::Server_UnEquipWeaponToScene_Implementation()
{
	UnEquipWeaponToScene();
}

bool UPackageComponent::Server_UnEquipWeaponToScene_Validate()
{
	return true;
}

void UPackageComponent::UnEquipWeaponToPackage()
{
	if (!GetOwner()->HasAuthority())
	{
		Server_UnEquipWeaponToPackage();
		return;
	}
	
	if (HoldWeapon)
	{
		const int32 AmmoItemId = HoldWeapon->GetAmmoItemId();
		const int32 LoadedAmmo = HoldWeapon->GetCurrentClipVolume();
		if (AmmoItemId != INDEX_NONE && LoadedAmmo > 0)
		{
			FItemStack AmmoStack;
			AmmoStack.ItemId = AmmoItemId;
			AmmoStack.Quantity = LoadedAmmo;
			if (!TryAddItemStack(AmmoStack))
			{
				return;
			}
			HoldWeapon->ExtractLoadedAmmo();
		}
		AddItemToPackage(HoldWeapon->GetID());
		HoldWeapon->Destroy();
		HoldWeapon = nullptr;
		GetOwner()->ForceNetUpdate();
	}
	if (OnUnEquipWeapon.IsBound())
	{
		OnUnEquipWeapon.Broadcast(-1);
	}
	
}

void UPackageComponent::Server_UnEquipWeaponToPackage_Implementation()
{
	UnEquipWeaponToPackage();
}

bool UPackageComponent::Server_UnEquipWeaponToPackage_Validate()
{
	return true;
}

void UPackageComponent::EquipWeaponFromPackage_Internal(int32 Key)
{
	const int32* ItemId = PackageMap.Find(Key);
	UPropsSubsystem* Props = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UPropsSubsystem>() : nullptr;
	if (!IsSurvivalMirrorPackageKey(Key) && ItemId && Props && Props->IsValidSurvivalWeaponDefinition(*ItemId)
		&& TryEquipWeapon(*ItemId))
	{
		int32 ID = 0;
		RemoveItemFromPackage(Key, ID);
	}
	
}

int32 UPackageComponent::FindLowestAvailableSurvivalSlot(const TArray<FItemStack>& Items) const
{
	int32 SlotId = 0;
	while (Items.ContainsByPredicate([SlotId](const FItemStack& Stack)
	{
		return Stack.SlotId == SlotId;
	}))
	{
		++SlotId;
	}
	return SlotId;
}

bool UPackageComponent::IsSurvivalResourceItem(int32 ItemId) const
{
	const UWorld* World = GetWorld();
	const UPropsSubsystem* Props = World ? World->GetGameInstance()->GetSubsystem<UPropsSubsystem>() : nullptr;
	return Props && Props->IsStackableSurvivalResourceItem(ItemId);
}

bool UPackageComponent::IsSurvivalMirrorPackageKey(int32 PackageKey) const
{
	return PackageKey >= SurvivalMirrorPackageKeyBase;
}

int32 UPackageComponent::GetSurvivalMirrorPackageKey(int32 SlotId) const
{
	return SlotId >= 0 && SlotId <= MAX_int32 - SurvivalMirrorPackageKeyBase
		? SurvivalMirrorPackageKeyBase + SlotId
		: INDEX_NONE;
}

void UPackageComponent::SynchronizeSurvivalPackageMap()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	for (auto It = PackageMap.CreateIterator(); It; ++It)
	{
		if (IsSurvivalMirrorPackageKey(It.Key()))
		{
			It.RemoveCurrent();
		}
	}

	for (const FItemStack& Stack : SurvivalItemStacks)
	{
		const int32 PackageKey = GetSurvivalMirrorPackageKey(Stack.SlotId);
		if (PackageKey != INDEX_NONE && Stack.IsValid() && IsSurvivalResourceItem(Stack.ItemId))
		{
			PackageMap.Add(PackageKey, Stack.ItemId);
		}
	}

	RebuildPackageSnapshot();
}

void UPackageComponent::NotifySurvivalInventoryChanged()
{
	SurvivalItemStacks.Sort([](const FItemStack& Left, const FItemStack& Right)
	{
		return Left.SlotId < Right.SlotId;
	});
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		SynchronizeSurvivalPackageMap();
	}
	SetSelectedSurvivalSlotId(SelectedSurvivalSlotId);
	OnSurvivalInventoryChanged.Broadcast(SurvivalItemStacks);
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		GetOwner()->ForceNetUpdate();
	}
}

void UPackageComponent::GetSurvivalInventoryItems(TArray<FSurvivalItemView>& OutItems) const
{
	OutItems.Reset();
	const UWorld* World = GetWorld();
	UPropsSubsystem* Props = World ? World->GetGameInstance()->GetSubsystem<UPropsSubsystem>() : nullptr;
	if (!Props)
	{
		return;
	}

	for (const FItemStack& Stack : SurvivalItemStacks)
	{
		FSurvivalItemView View;
		if (Props->GetSurvivalItemView(Stack.ItemId, View))
		{
			View.Stack = Stack;
			OutItems.Add(MoveTemp(View));
		}
	}
}

bool UPackageComponent::GetSurvivalInventoryItem(int32 SlotId, FSurvivalItemView& OutItem) const
{
	const FItemStack* Stack = SurvivalItemStacks.FindByPredicate([SlotId](const FItemStack& Candidate)
	{
		return Candidate.SlotId == SlotId;
	});
	if (!Stack)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	UPropsSubsystem* Props = World ? World->GetGameInstance()->GetSubsystem<UPropsSubsystem>() : nullptr;
	if (!Props || !Props->GetSurvivalItemView(Stack->ItemId, OutItem))
	{
		return false;
	}

	OutItem.Stack = *Stack;
	return true;
}

int32 UPackageComponent::GetItemQuantityByTag(FGameplayTag ItemTag) const
{
	if (!ItemTag.IsValid())
	{
		return 0;
	}

	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UPropsSubsystem* Props = GameInstance ? GameInstance->GetSubsystem<UPropsSubsystem>() : nullptr;
	if (!Props)
	{
		return 0;
	}

	int64 TotalQuantity = 0;
	for (const FItemStack& Stack : SurvivalItemStacks)
	{
		if (!Stack.IsValid())
		{
			continue;
		}

		FSurvivalItemView ItemView;
		if (Props->GetSurvivalItemView(Stack.ItemId, ItemView) && ItemView.ItemTags.HasTag(ItemTag))
		{
			TotalQuantity += Stack.Quantity;
			if (TotalQuantity >= MAX_int32)
			{
				return MAX_int32;
			}
		}
	}

	return static_cast<int32>(TotalQuantity);
}

bool UPackageComponent::TryConsumeItemsByTag(FGameplayTag ItemTag, int32 Quantity)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !ItemTag.IsValid() || Quantity <= 0)
	{
		return false;
	}

	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UPropsSubsystem* Props = GameInstance ? GameInstance->GetSubsystem<UPropsSubsystem>() : nullptr;
	if (!Props)
	{
		return false;
	}

	TArray<FItemStack> CandidateItems = SurvivalItemStacks;
	TArray<int32> MatchingIndices;
	int64 AvailableQuantity = 0;
	for (int32 Index = 0; Index < CandidateItems.Num(); ++Index)
	{
		const FItemStack& Stack = CandidateItems[Index];
		if (!Stack.IsValid())
		{
			continue;
		}

		FSurvivalItemView ItemView;
		if (Props->GetSurvivalItemView(Stack.ItemId, ItemView) && ItemView.ItemTags.HasTag(ItemTag))
		{
			MatchingIndices.Add(Index);
			AvailableQuantity += Stack.Quantity;
		}
	}

	if (AvailableQuantity < Quantity)
	{
		return false;
	}

	MatchingIndices.Sort([&CandidateItems](int32 LeftIndex, int32 RightIndex)
	{
		const int32 LeftSlotId = CandidateItems[LeftIndex].SlotId;
		const int32 RightSlotId = CandidateItems[RightIndex].SlotId;
		return LeftSlotId == RightSlotId ? LeftIndex < RightIndex : LeftSlotId < RightSlotId;
	});

	int32 RemainingQuantity = Quantity;
	for (const int32 Index : MatchingIndices)
	{
		if (RemainingQuantity <= 0)
		{
			break;
		}

		FItemStack& Stack = CandidateItems[Index];
		const int32 QuantityToConsume = FMath::Min(Stack.Quantity, RemainingQuantity);
		Stack.Quantity -= QuantityToConsume;
		RemainingQuantity -= QuantityToConsume;
	}

	if (RemainingQuantity != 0)
	{
		return false;
	}

	CandidateItems.RemoveAll([](const FItemStack& Stack)
	{
		return Stack.Quantity <= 0;
	});
	SurvivalItemStacks = MoveTemp(CandidateItems);
	NotifySurvivalInventoryChanged();
	return true;
}

bool UPackageComponent::GetSurvivalStackForPackageKey(int32 PackageKey, FItemStack& OutStack) const
{
	OutStack = FItemStack();
	if (!IsSurvivalMirrorPackageKey(PackageKey))
	{
		return false;
	}

	const int32 SlotId = PackageKey - SurvivalMirrorPackageKeyBase;
	const FItemStack* Stack = SurvivalItemStacks.FindByPredicate([SlotId](const FItemStack& Candidate)
	{
		return Candidate.SlotId == SlotId;
	});
	if (!Stack || !Stack->IsValid() || !IsSurvivalResourceItem(Stack->ItemId))
	{
		return false;
	}

	OutStack = *Stack;
	return true;
}

bool UPackageComponent::TryAddItemStack(const FItemStack& ItemStack)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !ItemStack.IsValid() || !IsSurvivalResourceItem(ItemStack.ItemId))
	{
		return false;
	}

	UPropsSubsystem* Props = GetWorld()->GetGameInstance()->GetSubsystem<UPropsSubsystem>();
	const int32 MaxStackSize = Props ? Props->GetMaxStackSize(ItemStack.ItemId) : 0;
	if (MaxStackSize <= 0)
	{
		return false;
	}

	TArray<FItemStack> CandidateItems = SurvivalItemStacks;
	int32 RemainingQuantity = ItemStack.Quantity;
	for (FItemStack& ExistingStack : CandidateItems)
	{
		if (ExistingStack.ItemId != ItemStack.ItemId || ExistingStack.Quantity >= MaxStackSize)
		{
			continue;
		}

		const int32 AddedQuantity = FMath::Min(RemainingQuantity, MaxStackSize - ExistingStack.Quantity);
		ExistingStack.Quantity += AddedQuantity;
		RemainingQuantity -= AddedQuantity;
		if (RemainingQuantity == 0)
		{
			break;
		}
	}

	while (RemainingQuantity > 0)
	{
		if (CandidateItems.Num() >= SurvivalMaxSlots)
		{
			return false;
		}

		FItemStack& NewStack = CandidateItems.AddDefaulted_GetRef();
		NewStack.SlotId = FindLowestAvailableSurvivalSlot(CandidateItems);
		NewStack.ItemId = ItemStack.ItemId;
		NewStack.Quantity = FMath::Min(RemainingQuantity, MaxStackSize);
		RemainingQuantity -= NewStack.Quantity;
	}

	SurvivalItemStacks = MoveTemp(CandidateItems);
	NotifySurvivalInventoryChanged();
	return true;
}

bool UPackageComponent::TryRemoveItemStack(int32 SlotId, int32 Quantity, FItemStack& RemovedStack)
{
	RemovedStack = FItemStack();
	if (!GetOwner() || !GetOwner()->HasAuthority() || Quantity <= 0)
	{
		return false;
	}

	const int32 StackIndex = SurvivalItemStacks.IndexOfByPredicate([SlotId](const FItemStack& Stack)
	{
		return Stack.SlotId == SlotId;
	});
	if (StackIndex == INDEX_NONE || SurvivalItemStacks[StackIndex].Quantity < Quantity)
	{
		return false;
	}

	FItemStack& SourceStack = SurvivalItemStacks[StackIndex];
	RemovedStack.SlotId = SlotId;
	RemovedStack.ItemId = SourceStack.ItemId;
	RemovedStack.Quantity = Quantity;
	SourceStack.Quantity -= Quantity;
	if (SourceStack.Quantity == 0)
	{
		SurvivalItemStacks.RemoveAt(StackIndex);
	}

	NotifySurvivalInventoryChanged();
	return true;
}

bool UPackageComponent::TryTransferItemStack(int32 SlotId, int32 Quantity, AActor* Destination)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Destination || Destination == GetOwner()
		|| !Destination->GetClass()->ImplementsInterface(USurvivalInventoryInterface::StaticClass()))
	{
		return false;
	}

	FItemStack RemovedStack;
	if (!TryRemoveItemStack(SlotId, Quantity, RemovedStack))
	{
		return false;
	}

	if (ISurvivalInventoryInterface::Execute_TryAddItemStack(Destination, RemovedStack))
	{
		return true;
	}

	TryAddItemStack(RemovedStack);
	return false;
}

void UPackageComponent::TransferAllSurvivalItemsTo(AActor* Destination)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const TArray<FItemStack> Snapshot = SurvivalItemStacks;
	for (const FItemStack& Stack : Snapshot)
	{
		TryTransferItemStack(Stack.SlotId, Stack.Quantity, Destination);
	}
}

void UPackageComponent::RequestTransferItemStack(int32 SlotId, int32 Quantity, AActor* Destination)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		TryTransferItemStack(SlotId, Quantity, Destination);
	}
	else
	{
		Server_RequestTransferItemStack(SlotId, Quantity, Destination);
	}
}

void UPackageComponent::RequestDropItemStack(int32 SlotId, int32 Quantity)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		FItemStack RemovedStack;
		if (TryRemoveItemStack(SlotId, Quantity, RemovedStack))
		{
			SpawnSceneItemActorFromPlayerNear(RemovedStack);
		}
	}
	else
	{
		Server_RequestDropItemStack(SlotId, Quantity);
	}
}

void UPackageComponent::RequestConsumeItemStack(int32 SlotId, int32 Quantity)
{
	if (!(GetOwner() && GetOwner()->HasAuthority()))
	{
		Server_RequestConsumeItemStack(SlotId, Quantity);
		return;
	}

	FSurvivalItemView ItemView;
	if (!GetSurvivalInventoryItem(SlotId, ItemView) || ItemView.Stack.Quantity < Quantity || Quantity <= 0)
	{
		return;
	}

	UPropsSubsystem* Props = GetWorld()->GetGameInstance()->GetSubsystem<UPropsSubsystem>();
	float HealthDelta = 0.0f;
	float HungerDelta = 0.0f;
	float ThirstDelta = 0.0f;
	if (!Props || !Props->GetConsumableEffects(ItemView.Stack.ItemId, HealthDelta, HungerDelta, ThirstDelta))
	{
		return;
	}

	if (ALgCharacterBase* Character = Cast<ALgCharacterBase>(GetOwner()))
	{
		if (USurvivalVitalsComponent* Vitals = Character->GetSurvivalVitalsComponent())
		{
			if (Vitals->ApplyConsumable(HealthDelta, HungerDelta, ThirstDelta, Quantity))
			{
				FItemStack RemovedStack;
				TryRemoveItemStack(SlotId, Quantity, RemovedStack);
			}
		}
	}
}

void UPackageComponent::SetSelectedSurvivalSlotId(int32 SlotId)
{
	const bool bIsValidSelection = SurvivalItemStacks.ContainsByPredicate([this, SlotId](const FItemStack& Stack)
	{
		return Stack.SlotId == SlotId && Stack.IsValid() && IsSurvivalResourceItem(Stack.ItemId);
	});

	const int32 NewSelectedSlotId = bIsValidSelection ? SlotId : INDEX_NONE;
	if (SelectedSurvivalSlotId != NewSelectedSlotId)
	{
		SelectedSurvivalSlotId = NewSelectedSlotId;
		OnSurvivalInventorySelectionChanged.Broadcast(SelectedSurvivalSlotId);
	}
}

void UPackageComponent::RequestInteract(AActor* Target)
{
	if (!(GetOwner() && GetOwner()->HasAuthority()))
	{
		Server_RequestInteract(Target);
		return;
	}

	if (!Target || !Target->GetClass()->ImplementsInterface(USurvivalInteractableInterface::StaticClass())
		|| FVector::DistSquared(Target->GetActorLocation(), GetOwner()->GetActorLocation()) > FMath::Square(250.0f))
	{
		return;
	}

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	if (InstigatorPawn && ISurvivalInteractableInterface::Execute_CanInteract(Target, InstigatorPawn))
	{
		ISurvivalInteractableInterface::Execute_Interact(Target, InstigatorPawn);
	}
}

bool UPackageComponent::ConsumeItemById(int32 ItemId, int32 Quantity)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Quantity <= 0)
	{
		return false;
	}

	int32 RemainingQuantity = Quantity;
	for (const FItemStack& Stack : SurvivalItemStacks)
	{
		if (Stack.ItemId == ItemId)
		{
			RemainingQuantity -= Stack.Quantity;
		}
	}
	if (RemainingQuantity > 0)
	{
		return false;
	}

	RemainingQuantity = Quantity;
	const TArray<FItemStack> Snapshot = SurvivalItemStacks;
	for (const FItemStack& Stack : Snapshot)
	{
		if (Stack.ItemId != ItemId || RemainingQuantity <= 0)
		{
			continue;
		}
		FItemStack RemovedStack;
		const int32 ToRemove = FMath::Min(Stack.Quantity, RemainingQuantity);
		if (!TryRemoveItemStack(Stack.SlotId, ToRemove, RemovedStack))
		{
			return false;
		}
		RemainingQuantity -= ToRemove;
	}
	return RemainingQuantity == 0;
}

int32 UPackageComponent::GetItemQuantityById(int32 ItemId) const
{
	int32 TotalQuantity = 0;
	for (const FItemStack& Stack : SurvivalItemStacks)
	{
		if (Stack.ItemId == ItemId)
		{
			TotalQuantity += Stack.Quantity;
		}
	}
	return TotalQuantity;
}

bool UPackageComponent::TryCraftRecipe(const FSurvivalRecipeDefinition& Recipe, int32 CraftCount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Recipe.RecipeId.IsNone() || CraftCount <= 0
		|| Recipe.Ingredients.IsEmpty() || Recipe.Results.IsEmpty())
	{
		return false;
	}

	UPropsSubsystem* Props = GetWorld()->GetGameInstance()->GetSubsystem<UPropsSubsystem>();
	if (!Props)
	{
		return false;
	}

	TArray<FItemStack> CandidateItems = SurvivalItemStacks;
	TMap<int32, int32> CandidatePackageItems = PackageMap;
	for (const FItemStack& Ingredient : Recipe.Ingredients)
	{
		if (!Ingredient.IsValid() || !IsSurvivalResourceItem(Ingredient.ItemId)
			|| Ingredient.Quantity > MAX_int32 / CraftCount)
		{
			return false;
		}

		int32 RemainingQuantity = Ingredient.Quantity * CraftCount;
		for (int32 Index = CandidateItems.Num() - 1; Index >= 0 && RemainingQuantity > 0; --Index)
		{
			FItemStack& Stack = CandidateItems[Index];
			if (Stack.ItemId != Ingredient.ItemId)
			{
				continue;
			}

			const int32 ConsumedQuantity = FMath::Min(Stack.Quantity, RemainingQuantity);
			Stack.Quantity -= ConsumedQuantity;
			RemainingQuantity -= ConsumedQuantity;
			if (Stack.Quantity == 0)
			{
				CandidateItems.RemoveAt(Index);
			}
		}

		if (RemainingQuantity > 0)
		{
			return false;
		}
	}

	auto FindLowestCandidatePackageKey = [](const TMap<int32, int32>& Items)
	{
		int32 Key = 0;
		while (Items.Contains(Key))
		{
			++Key;
		}
		return Key;
	};

	for (const FItemStack& Result : Recipe.Results)
	{
		if (!Result.IsValid() || Result.Quantity > MAX_int32 / CraftCount)
		{
			return false;
		}

		const int32 ResultQuantity = Result.Quantity * CraftCount;
		if (IsSurvivalResourceItem(Result.ItemId))
		{
			const int32 MaxStackSize = Props->GetMaxStackSize(Result.ItemId);
			if (MaxStackSize <= 0)
			{
				return false;
			}

			int32 RemainingQuantity = ResultQuantity;
			for (FItemStack& ExistingStack : CandidateItems)
			{
				if (ExistingStack.ItemId != Result.ItemId || ExistingStack.Quantity >= MaxStackSize)
				{
					continue;
				}

				const int32 AddedQuantity = FMath::Min(RemainingQuantity, MaxStackSize - ExistingStack.Quantity);
				ExistingStack.Quantity += AddedQuantity;
				RemainingQuantity -= AddedQuantity;
				if (RemainingQuantity == 0)
				{
					break;
				}
			}

			while (RemainingQuantity > 0)
			{
				if (CandidateItems.Num() >= SurvivalMaxSlots)
				{
					return false;
				}

				FItemStack NewStack;
				NewStack.SlotId = FindLowestAvailableSurvivalSlot(CandidateItems);
				NewStack.ItemId = Result.ItemId;
				NewStack.Quantity = FMath::Min(RemainingQuantity, MaxStackSize);
				CandidateItems.Add(NewStack);
				RemainingQuantity -= NewStack.Quantity;
			}
			continue;
		}

		if (!Props->IsValidSurvivalWeaponDefinition(Result.ItemId))
		{
			return false;
		}

		for (int32 WeaponIndex = 0; WeaponIndex < ResultQuantity; ++WeaponIndex)
		{
			CandidatePackageItems.Add(FindLowestCandidatePackageKey(CandidatePackageItems), Result.ItemId);
		}
	}

	SurvivalItemStacks = MoveTemp(CandidateItems);
	PackageMap = MoveTemp(CandidatePackageItems);
	NotifySurvivalInventoryChanged();
	return true;
}

void UPackageComponent::DropAllSurvivalItems()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	for (const FItemStack& Stack : SurvivalItemStacks)
	{
		SpawnSceneItemActorFromPlayerNear(Stack);
	}
	SurvivalItemStacks.Reset();
	NotifySurvivalInventoryChanged();

	for (const TPair<int32, int32>& Item : PackageMap)
	{
		if (IsSurvivalMirrorPackageKey(Item.Key))
		{
			continue;
		}

		FItemStack Stack;
		Stack.ItemId = Item.Value;
		Stack.Quantity = 1;
		SpawnSceneItemActorFromPlayerNear(Stack);
	}
	PackageMap.Reset();
	RebuildPackageSnapshot();

	for (const TPair<ESkinType, int32>& Skin : SkinMap)
	{
		FItemStack Stack;
		Stack.ItemId = Skin.Value;
		Stack.Quantity = 1;
		SpawnSceneItemActorFromPlayerNear(Stack);
	}
	SkinMap.Reset();
	RebuildSkinSnapshot();

	if (HoldWeapon)
	{
		const int32 AmmoItemId = HoldWeapon->GetAmmoItemId();
		const int32 LoadedAmmo = HoldWeapon->ExtractLoadedAmmo();
		if (AmmoItemId != INDEX_NONE && LoadedAmmo > 0)
		{
			FItemStack AmmoStack;
			AmmoStack.ItemId = AmmoItemId;
			AmmoStack.Quantity = LoadedAmmo;
			SpawnSceneItemActorFromPlayerNear(AmmoStack);
		}

		FItemStack Stack;
		Stack.ItemId = HoldWeapon->GetID();
		Stack.Quantity = 1;
		SpawnSceneItemActorFromPlayerNear(Stack);
		HoldWeapon->Destroy();
		HoldWeapon = nullptr;
		GetOwner()->ForceNetUpdate();
	}
}

ASceneItemActor* UPackageComponent::SpawnSceneItemActorFromPlayerNear(const FItemStack& ItemStack)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !ItemStack.IsValid())
	{
		return nullptr;
	}

	FVector NewDirection = GetOwner()->GetActorForwardVector().RotateAngleAxis(FMath::FRandRange(-90.f, 90.f), FVector::UpVector);
	FVector Location = GetOwner()->GetActorLocation() + NewDirection * FMath::FRandRange(100.f, 150.f);
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Location, Location + FVector::DownVector * 500.0f, ECC_Visibility))
	{
		Location = Hit.Location;
	}

	FTransform Transform;
	Transform.SetLocation(Location);
	ASceneItemActor* SceneItemActor = GetWorld()->SpawnActorDeferred<ASceneItemActor>(ASceneItemActor::StaticClass(), Transform);
	if (SceneItemActor)
	{
		SceneItemActor->SetItemStack(ItemStack);
		SceneItemActor->FinishSpawning(Transform);
	}
	return SceneItemActor;
}

void UPackageComponent::Server_RequestTransferItemStack_Implementation(int32 SlotId, int32 Quantity, AActor* Destination)
{
	TryTransferItemStack(SlotId, Quantity, Destination);
}

void UPackageComponent::Server_RequestDropItemStack_Implementation(int32 SlotId, int32 Quantity)
{
	RequestDropItemStack(SlotId, Quantity);
}

void UPackageComponent::Server_RequestConsumeItemStack_Implementation(int32 SlotId, int32 Quantity)
{
	RequestConsumeItemStack(SlotId, Quantity);
}

void UPackageComponent::Server_RequestInteract_Implementation(AActor* Target)
{
	RequestInteract(Target);
}


