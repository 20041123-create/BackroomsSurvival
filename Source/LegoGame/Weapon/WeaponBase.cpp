// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LegoGame/LegoGame.h"
#include "LegoGame/Character/LgCharacterBase.h"
#include "Sound/SoundCue.h"
#include "Engine/DamageEvents.h"
#include "Net/UnrealNetwork.h"


// Sets default values
AWeaponBase::AWeaponBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	WeaponMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	SetRootComponent(WeaponMeshComponent);
	
	MaxClipVolume = 30;
	FireInterval = 0.1f;
	ShotDistance = 100000.f;
	ID = -1;
	
	//开启网络同步
	bReplicates = true;
	//设置枪械使用Owner的相关性
	bNetUseOwnerRelevancy = true;
}

// Called when the game starts or when spawned
void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	CurrClipVolume = MaxClipVolume;
}

void AWeaponBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWeaponBase, ID);
	DOREPLIFETIME(AWeaponBase, MyMaster);
	
}

void AWeaponBase::SpawnBullet()
{
	if (CurrClipVolume==0)
	{
		return;
	}
	CurrentState = EWeaponState::EWS_Firing;
	SpawnDamage();
	SpawnEffect();
	
	if (--CurrClipVolume>0)
	{
		//继续定时发射子弹
		GetWorld()->GetTimerManager().SetTimer(TimerHandle,this,&ThisClass::SpawnBullet,FireInterval);
	}
	else
	{
		CurrentState = EWeaponState::EWS_Empty;
	}
	LastFireTime = GetWorld()->GetTimeSeconds();
	
	if (OnWeaponClipChanged.IsBound())
	{
		OnWeaponClipChanged.Broadcast(CurrClipVolume,MaxClipVolume);
	}
}

void AWeaponBase::SpawnDamage()
{
	FVector Position;
	FVector Direction;
	GetFirstStartPositionAndDirection(Position,Direction);
	
	//发射射线
	FHitResult Hit;
	//如何设置忽略对象
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(MyMaster);
	
	if (GetWorld()->LineTraceSingleByChannel(Hit,Position,Position+Direction*ShotDistance,WEAPON_TRACE,CollisionParams))
	{
		//UE_LOG(LogTemp, Warning, TEXT("%s"),*Hit.GetActor()->GetName());
		//造成伤害
		FPointDamageEvent DamageEvent;
		Hit.GetActor()->TakeDamage(1,DamageEvent,MyMaster->GetController(),this);
	}
	//绘制调试线
	DrawDebugLine(GetWorld(),Position,Position+Direction*ShotDistance,FColor::Purple,false,3.f);
}

void AWeaponBase::SpawnEffect()
{
	//特效和3D声音
	if (FireSound)
	{
		//播放声音
		UGameplayStatics::PlaySoundAtLocation(GetWorld(),FireSound,GetActorLocation());
	}
	if (FireParticle)
	{
		//播放特效
		UGameplayStatics::SpawnEmitterAttached(FireParticle,WeaponMeshComponent,TEXT("Muzzle"));
	}
}

void AWeaponBase::GetFirstStartPositionAndDirection(FVector& Position, FVector& Direction)
{
	//默认取一下枪口位置和角度
	Position = WeaponMeshComponent->GetSocketLocation(TEXT("Muzzle"));
	Direction = WeaponMeshComponent->GetSocketQuaternion(TEXT("Muzzle")).GetAxisX();
	//判断是AI还是玩家
	if (APlayerController* PC = Cast<APlayerController>(MyMaster->GetController()))
	{
		int32 SizeX = 0;
		int32 SizeY = 0;
		PC->GetViewportSize(SizeX,SizeY);
		float CenterX = SizeX/2.0f;
		float CenterY = SizeY/2.0f;
		//将2D坐标转换为3D坐标  得到3D空间中的位置和法向量
		FVector ProjectPosition;
		FVector ProjectDirection;
		if (PC->DeprojectScreenPositionToWorld(CenterX,CenterY,ProjectPosition,ProjectDirection))
		{
			//发射射线，通过屏幕准心找到击中点，然后从枪口位置向中心点发射射线
			FHitResult Hit;
			FCollisionQueryParams CollisionParams;
			CollisionParams.AddIgnoredActor(MyMaster);
			if (GetWorld()->LineTraceSingleByChannel(Hit,ProjectPosition,ProjectPosition+ProjectDirection*ShotDistance,WEAPON_TRACE,CollisionParams))
			{
				//如果检测到目标说明从屏幕中心有瞄准到内容
				//优先判断击中位置是否在枪口后方
				
				
				FVector HitDirection = (Hit.Location-Position).GetSafeNormal();
				if (FVector::DotProduct(HitDirection,Direction)>0)
				{
					Direction = HitDirection;
				}
			}
			//DrawDebugLine(GetWorld(),ProjectPosition,ProjectPosition+ProjectDirection*ShotDistance,FColor::Blue,false,3.f);
		}
		
	}
	else
	{
		
	}
}

// Called every frame
void AWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeaponBase::SetMaster(ALgCharacterBase* InMaster)
{
	if (!InMaster)
	{
		return;
	}
	MyMaster = InMaster;
	//设置依附关系
	WeaponMeshComponent->AttachToComponent(MyMaster->GetMesh(),FAttachmentTransformRules::SnapToTargetNotIncludingScale,TEXT("WeaponSocket"));
	
	//设置owner
	if (HasAuthority())
	{
		SetOwner(MyMaster);
	}
	
	
}

void AWeaponBase::StartFire()
{
	if (CurrentState == EWeaponState::EWS_Normal)
	{
		if (GetWorld()->GetTimeSeconds()-LastFireTime<FireInterval)//小于间隔时间不能发射
		{
			return;
		}
		SpawnBullet();
	}
	
}

void AWeaponBase::StopFire()
{
	//检查定时任务
	
	if (TimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	}
	if (CurrentState == EWeaponState::EWS_Firing)
	{
		CurrentState = EWeaponState::EWS_Normal;
	}
}

void AWeaponBase::ReloadClip()
{
	if (CurrentState == EWeaponState::EWS_Reloading)//如果当前在换子弹则不要进入更换弹夹
	{
		return;
	}
	CurrentState = EWeaponState::EWS_Reloading;
	StopFire();
	
	if (ReloadMontage&&MyMaster)
	{
		MyMaster->PlayAnimMontage(ReloadMontage,1,MyMaster->IsIronSight() ? TEXT("Default") : TEXT("IronSights"));
	}
	
}

void AWeaponBase::MakeFullClip()
{
	//UE_LOG(LogTemp, Warning, TEXT("re"));
	CurrClipVolume = MaxClipVolume;
	
	if (OnWeaponClipChanged.IsBound())
	{
		OnWeaponClipChanged.Broadcast(CurrClipVolume,MaxClipVolume);
	}
	CurrentState = EWeaponState::EWS_Normal;
}

