// Fill out your copyright notice in the Description page of Project Settings.


#include "LgCharacterBase.h"

#include "Components/BillboardComponent.h"
#include "LegoGame/LegoGame.h"
#include "LegoGame/Components/LgCharacterMovementComponent.h"
#include "LegoGame/Components/PackageComponent.h"
#include "LegoGame/Components/SkinComponent.h"
#include "LegoGame/GamePlay/MainGame/LgPlayerState.h"
#include "LegoGame/Weapon/WeaponBase.h"
#include "Net/UnrealNetwork.h"


// Sets default values
ALgCharacterBase::ALgCharacterBase(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer.SetDefaultSubobjectClass<ULgCharacterMovementComponent>(CharacterMovementComponentName))//替换父类中已经添加的父类的名字
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	PackageComponent = CreateDefaultSubobject<UPackageComponent>(TEXT("PackageComponent"));//功能组件没有坐标关系，不需要setupattachment
	
	SkinComponent = CreateDefaultSubobject<USkinComponent>(TEXT("SkinComponent"));
	
	//关闭角色跟随控制器旋转
	bUseControllerRotationYaw = false;
	
	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("BillboardComponent"));
	BillboardComponent->SetupAttachment(RootComponent);
	BillboardComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 150.f));
}

// Called when the game starts or when spawned
void ALgCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	//绑定换装代理
	if (PackageComponent&&SkinComponent)
	{
		PackageComponent->OnPutOnSkin.AddUObject(SkinComponent,&USkinComponent::OnPutOnSkin);
		PackageComponent->OnTakeOffSkin.AddUObject(SkinComponent,&USkinComponent::OnTakeOffSkin);
		
		//装卸武器的绑定通知
		PackageComponent->OnEquipWeapon.AddUObject(this,&ThisClass::OnEquipWeapon);
		PackageComponent->OnUnEquipWeapon.AddUObject(this,&ThisClass::OnUnEquipWeapon);
	}
	//UpdateMeshCount = 6;
	
}

void ALgCharacterBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALgCharacterBase, bSprinting);
	DOREPLIFETIME(ALgCharacterBase, bIronSight);
	
	
}

// 当服务端正式拥有此角色时（此时CopyProperties已完毕）
void ALgCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	// 服务端更换衣服
	UpdateJobMesh();
}

// 当客户端的网络握手完成，正式得知这个 Character 对应哪个 PlayerState 时
void ALgCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	// 客户端更换衣服（无论是自己还是别的模拟客户端，都会触发这里）
	UpdateJobMesh();
}

void ALgCharacterBase::GetOwnedGameplayTags(FGameplayTagContainer& OutTagContainer) const
{
	OutTagContainer = TagContainer;
	
}


// Called every frame
void ALgCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ALgCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}



float ALgCharacterBase::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
	class AController* EventInstigator, AActor* DamageCauser)
{
	
	UE_LOG(LogTemp, Warning, TEXT("我是%s,我被%s攻击了"),*GetName(),*EventInstigator->GetPawn()->GetName());
	
	if (OnReceiveDamage.IsBound())
	{
		OnReceiveDamage.Broadcast(Cast<ALgCharacterBase>(EventInstigator->GetPawn()));
	}
	
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	
}

void ALgCharacterBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	//在编辑模式下，当修改对象属性时此函数调用
	if (BillboardComponent)
	{
		const TCHAR* IconSpriteSource = TEXT("/Script/Engine.Texture2D'/Game/LegoGame/Textures/UI/ICons/icon_weapon_Apple.icon_weapon_Apple'");
		if (TeamType==ETeamType::ETT_Police)
		{
			IconSpriteSource = TEXT("/Script/Engine.Texture2D'/Game/LegoGame/Textures/UI/ICons/icon_weapon_Pan.icon_weapon_Pan'");
		}
		else if (TeamType==ETeamType::ETT_Bandit)
		{
			IconSpriteSource = TEXT("/Script/Engine.Texture2D'/Game/LegoGame/Textures/UI/ICons/icon_weapon_Grenade.icon_weapon_Grenade'");
		}
		
		
		
		UTexture2D* IconSprite = LoadObject<UTexture2D>(BillboardComponent,IconSpriteSource);
		//修改广告牌图片内容
		BillboardComponent->SetSprite(IconSprite);
	}
	
}


bool ALgCharacterBase::IsSprinting() const
{
	if (PackageComponent && PackageComponent->GetHoldWeapon())
	{
		//持枪只能朝前冲
		return bSprinting && FVector::DotProduct(GetVelocity().GetSafeNormal(), GetActorForwardVector())>0.9f&&!IsIronSight();
	}
	return bSprinting;
}


bool ALgCharacterBase::IsJumping() const
{
	//是否在跳跃中
	return GetCharacterMovement()->IsFalling();
}

TObjectPtr<AWeaponBase> ALgCharacterBase::GetHoldWeapon() const
{
	if (PackageComponent)
	{
		return PackageComponent->GetHoldWeapon();
	}
	return nullptr;
}

void ALgCharacterBase::StartFire()
{
	if (PackageComponent&&PackageComponent->GetHoldWeapon()&&IsIronSight())
	{
		PackageComponent->GetHoldWeapon()->StartFire();
	}
}

void ALgCharacterBase::StopFire()
{
	if (PackageComponent&&PackageComponent->GetHoldWeapon()&&IsIronSight())
	{
		PackageComponent->GetHoldWeapon()->StopFire();
	}
}

void ALgCharacterBase::StartIronSight()
{
	if (PackageComponent&&PackageComponent->GetHoldWeapon())
	{
		bIronSight = true;
	}
	
}

void ALgCharacterBase::StopIronSight()
{
	bIronSight = false;
	
}

void ALgCharacterBase::StartSprint()
{
	bSprinting = true;
	if (!HasAuthority())
	{
		Server_SetSprint(true);
	}
	//UE_LOG(LogTemp, Warning, TEXT("startSP"));
}

void ALgCharacterBase::StopSprint()
{
	bSprinting = false;
	if (!HasAuthority())
	{
		Server_SetSprint(false);
	}
	//UE_LOG(LogTemp, Warning, TEXT("stopSp"));
}

void ALgCharacterBase::DoCrouch()
{
	//UE_LOG(LogTemp, Warning, TEXT("crouch"));
	//当玩家角色没有蹲下的时候我们蹲下，否则站起
	if (CanCrouch())
	{
		Crouch();
	}
	else
	{
		UnCrouch();
	}
}

void ALgCharacterBase::OnEquipWeapon(int32 ID)
{
	//锁定视角
	bUseControllerRotationYaw = true;
	//关闭朝着加速度旋转
	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void ALgCharacterBase::OnUnEquipWeapon(int32 ID)
{
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
}

void ALgCharacterBase::ReloadWeapon()
{
	if (PackageComponent&&PackageComponent->GetHoldWeapon())
	{
		PackageComponent->GetHoldWeapon()->ReloadClip();
	}
	
}

void ALgCharacterBase::Server_SetSprint_Implementation(bool bNewSprint)
{
	if (bNewSprint)
	{
		StartSprint();
	}
	else
	{
		StopSprint();
	}
}

bool ALgCharacterBase::Server_SetSprint_Validate(bool bNewSprint)
{
	return true;
}

void ALgCharacterBase::UpdateJobMesh()
{
	if (ALgPlayerState* Ps = Cast<ALgPlayerState>(GetPlayerState()))
	{
		//防止服务器没有同步数据过来
		if (Ps->GetTeamType() == ETeamType::ETT_None || Ps->GetJobType() == EJobType::EJT_None)
		{
			//启动定时器，过一会再来
			// if (--UpdateMeshCount > 0)
			// {
			// 	FTimerHandle TimerHandle;
			// 	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ALgCharacterBase::UpdateJobMesh,0.5f);
			// }
			return;
		}
		const TCHAR* MeshSource = LG::GetJobMeshSource(Ps->GetTeamType(),Ps->GetJobType());
		USkeletalMesh* SkeletalMesh = LoadObject<USkeletalMesh>(GetMesh(), MeshSource);
		GetMesh()->SetSkeletalMesh(SkeletalMesh);
	}
	
}

// bool ALgCharacterBase::IsCrouched() const
// {
// 	return bIsCrouched;
// }


