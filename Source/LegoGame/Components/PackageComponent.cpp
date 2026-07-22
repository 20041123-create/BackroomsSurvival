// Fill out your copyright notice in the Description page of Project Settings.


#include "PackageComponent.h"

#include "LegoGame/LegoGame.h"
#include "LegoGame/Character/LgCharacterBase.h"
#include "LegoGame/Scene/SceneItemActor.h"
#include "LegoGame/Subsystem/PropsSubsystem.h"
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
		AddItemToPackage(SceneItemActorActor->GetID());
		//道具拾取完毕，移除地面上的Actor
		SceneItemActorActor->Destroy();
	}
	
}


void UPackageComponent::AddItemToPackage(int32 ID)
{
	//将Id记录到我们的Map容器中
	const int32 Key = AllowPackageKey();
	PackageMap.Add(Key,ID);
	Client_OnAddItemToPackage(Key,ID);
}

void UPackageComponent::Client_OnAddItemToPackage_Implementation(int32 Key, int32 ID)
{
	//对于客户端来说没有背包数据
	if (!GetOwner()->HasAuthority())
	{
		if (!PackageMap.Contains(Key))
		{
			PackageMap.Add(Key,ID);
		}
	}
	
	if (OnAddItemToPackage.IsBound())
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
		Client_OnRemoveItemFromPackage(Key,ID);
		return true;
	}
	return false;
}

void UPackageComponent::Client_OnRemoveItemFromPackage_Implementation(int32 Key, int32 ID)
{
	//广播
	if (OnRemoveItemFromPackage.IsBound())
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
	
	int32 ID = 0;
	if (RemoveItemFromPackage(Key,ID))//成功表明删除完毕
	{
		//从地面附近放置道具
		SpawnSceneItemActorFromPlayerNear(ID);
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

void UPackageComponent::SpawnSceneItemActorFromPlayerNear(int32 ID)
{
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
	Multi_OnPutOnSkin(SkinType,ID);
	
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
	//装备武器
	const FPropsBase* PropsBase = GetWorld()->GetGameInstance()->GetSubsystem<UPropsSubsystem>()->GetPropsById(ID);
	if (!PropsBase || PropsBase->Type!=EPropsType::EPT_Weapon)
	{
		return;
	}
	const FWeaponBaseHeader* WeaponBaseHeader = static_cast<const FWeaponBaseHeader*>(PropsBase);
	//创建枪械对象
	//检查手上是否有武器，如果有武器要先卸载武器
	if (HoldWeapon)
	{
		AddItemToPackage(HoldWeapon->GetID());
		HoldWeapon->Destroy();
	}
	//【关键修复1：防止0,0,0位置剔除，且第一时间赋予Owner】
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 在玩家当前位置生成，确保网络相关性立即生效
	FTransform SpawnTransform = GetOwner()->GetActorTransform();
	HoldWeapon = GetWorld()->SpawnActor<AWeaponBase>(WeaponBaseHeader->WeaponClass, SpawnTransform, SpawnParams);
	//创建枪械实例
	//HoldWeapon = GetWorld()->SpawnActor<AWeaponBase>(WeaponBaseHeader->WeaponClass);//创建了枪械的实例
	HoldWeapon->SetID(ID);//一定要给ID，否则卸载武器无法添加到背包内
	HoldWeapon->SetMaster(Cast<ALgCharacterBase>(GetOwner()));
	//【关键修复2：服务器直接在本地广播，不要使用 Multi RPC】
	if (OnEquipWeapon.IsBound())
	{
		//UE_LOG(LogTemp, Warning, TEXT("yesyesyes"));
		OnEquipWeapon.Broadcast(ID);
	}
	
	//Multi_OnEquipWeapon(ID);
	
	//OnRep_HoldWeapon();
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
	return (SceneItemActor->GetActorLocation() - GetOwner()->GetActorLocation()).Length() <= 250;
	
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
	if (IsValid(SceneItemActor)&&PutOnSkin(SceneItemActor->GetID(),SkinType))
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
	//处理脱掉装饰性道具
	int32 ID = TakeOffSkin(SkinType);
	if (ID>=0)
	{
		AddItemToPackage(ID);
	}
	
}

void UPackageComponent::TakeOffToScene(ESkinType SkinType)
{
	int32 ID = TakeOffSkin(SkinType);
	if (ID>=0)
	{
		SpawnSceneItemActorFromPlayerNear(ID);
	}
	
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
	
	if (IsValid(SceneItemActor)&&SceneItemActor->GetID()>=WEAPON_INDEX)
	{
		//装备武器
		EquipWeapon(SceneItemActor->GetID());
		SceneItemActor->Destroy();
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
		SpawnSceneItemActorFromPlayerNear(HoldWeapon->GetID());
		HoldWeapon->Destroy();
		HoldWeapon = nullptr;
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
		AddItemToPackage(HoldWeapon->GetID());
		HoldWeapon->Destroy();
		HoldWeapon = nullptr;
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
	if (PackageMap.Contains(Key) && PackageMap[Key] >= WEAPON_INDEX)
	{
		EquipWeapon(PackageMap[Key]);
		int32 ID = 0;
		RemoveItemFromPackage(Key, ID);
	}
	
}


