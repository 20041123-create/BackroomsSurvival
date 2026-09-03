#include "WeaponBase.h"

#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "LegoGame/LegoGame.h"
#include "LegoGame/Character/LgCharacterBase.h"
#include "LegoGame/Components/PackageComponent.h"
#include "LegoGame/Subsystem/PropsSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundCue.h"
#include "TimerManager.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	WeaponMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	SetRootComponent(WeaponMeshComponent);

	MaxClipVolume = 30;
	CurrClipVolume = MaxClipVolume;
	FireInterval = 0.1f;
	ShotDistance = 100000.0f;
	ID = INDEX_NONE;
	CurrentState = EWeaponState::EWS_Normal;

	bReplicates = true;
	bNetUseOwnerRelevancy = true;
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		CurrClipVolume = MaxClipVolume;
		CurrentState = EWeaponState::EWS_Normal;
	}
}

void AWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWeaponBase, ID);
	DOREPLIFETIME(AWeaponBase, MyMaster);
	DOREPLIFETIME_CONDITION(AWeaponBase, CurrClipVolume, COND_OwnerOnly);
	DOREPLIFETIME(AWeaponBase, CurrentState);
}

void AWeaponBase::RequestFireShot()
{
	if (!bWantsToFire || !MyMaster
		|| CurrentState == EWeaponState::EWS_Reloading
		|| CurrentState == EWeaponState::EWS_Empty)
	{
		return;
	}

	FVector ViewOrigin;
	FVector ViewDirection;
	GetFireViewPoint(ViewOrigin, ViewDirection);

	if (HasAuthority())
	{
		ProcessFireShot(ViewOrigin, ViewDirection);
	}
	else
	{
		// Owning clients predict only the cosmetic feedback. Ammo and damage remain authoritative.
		SpawnEffect();
		Server_RequestFireShot(ViewOrigin, ViewDirection);
	}
}

void AWeaponBase::ProcessFireShot(const FVector& ViewOrigin, const FVector& ViewDirection)
{
	if (!HasAuthority() || !MyMaster || GetOwner() != MyMaster || !MyMaster->IsIronSight()
		|| CurrentState == EWeaponState::EWS_Reloading)
	{
		return;
	}

	if (CurrClipVolume <= 0)
	{
		CurrClipVolume = 0;
		CurrentState = EWeaponState::EWS_Empty;
		bWantsToFire = false;
		GetWorldTimerManager().ClearTimer(FireTimerHandle);
		ForceNetUpdate();
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (LastFireTime > 0.0f && CurrentTime - LastFireTime + KINDA_SMALL_NUMBER < FireInterval)
	{
		return;
	}

	const FVector NormalizedViewDirection = ViewDirection.GetSafeNormal();
	if (NormalizedViewDirection.IsNearlyZero())
	{
		return;
	}

	// Player requests are checked against the server's pawn view/control rotation.
	if (Cast<APlayerController>(MyMaster->GetController()))
	{
		const FVector ServerViewOrigin = MyMaster->GetPawnViewLocation();
		if (FVector::DistSquared(ViewOrigin, ServerViewOrigin)
			> FMath::Square(MaxClientViewOriginError))
		{
			return;
		}

		const FVector ServerAimDirection = MyMaster->GetBaseAimRotation().Vector().GetSafeNormal();
		const float MinimumAimDot = FMath::Cos(FMath::DegreesToRadians(MaxClientAimErrorDegrees));
		if (FVector::DotProduct(ServerAimDirection, NormalizedViewDirection) < MinimumAimDot)
		{
			return;
		}
	}

	CurrentState = EWeaponState::EWS_Firing;
	SpawnDamage(ViewOrigin, NormalizedViewDirection);
	--CurrClipVolume;
	LastFireTime = CurrentTime;
	Multicast_PlayFireEffect();

	if (CurrClipVolume <= 0)
	{
		CurrClipVolume = 0;
		CurrentState = EWeaponState::EWS_Empty;
		bWantsToFire = false;
		GetWorldTimerManager().ClearTimer(FireTimerHandle);
	}

	if (OnWeaponClipChanged.IsBound())
	{
		OnWeaponClipChanged.Broadcast(CurrClipVolume, MaxClipVolume);
	}
	ForceNetUpdate();
}

void AWeaponBase::SpawnDamage(const FVector& ViewOrigin, const FVector& ViewDirection)
{
	if (!HasAuthority() || !MyMaster)
	{
		return;
	}

	const FVector MuzzlePosition = WeaponMeshComponent->GetSocketLocation(TEXT("Muzzle"));
	const FVector MuzzleForward = WeaponMeshComponent->GetSocketQuaternion(TEXT("Muzzle")).GetAxisX();

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(MyMaster);
	CollisionParams.AddIgnoredActor(this);

	FHitResult AimHit;
	const FVector ViewEnd = ViewOrigin + ViewDirection * ShotDistance;
	const bool bAimHit = GetWorld()->LineTraceSingleByChannel(
		AimHit, ViewOrigin, ViewEnd, WEAPON_TRACE, CollisionParams);
	const FVector AimPoint = bAimHit ? AimHit.ImpactPoint : ViewEnd;

	FVector ShotDirection = (AimPoint - MuzzlePosition).GetSafeNormal();
	if (ShotDirection.IsNearlyZero() || FVector::DotProduct(ShotDirection, MuzzleForward) <= 0.0f)
	{
		ShotDirection = MuzzleForward;
	}

	FHitResult WeaponHit;
	if (GetWorld()->LineTraceSingleByChannel(
		WeaponHit,
		MuzzlePosition,
		MuzzlePosition + ShotDirection * ShotDistance,
		WEAPON_TRACE,
		CollisionParams))
	{
		if (AActor* HitActor = WeaponHit.GetActor())
		{
			FPointDamageEvent DamageEvent;
			HitActor->TakeDamage(DamagePerShot, DamageEvent, MyMaster->GetController(), this);
		}
	}
}

void AWeaponBase::SpawnEffect()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), FireSound, GetActorLocation());
	}
	if (FireParticle)
	{
		UGameplayStatics::SpawnEmitterAttached(FireParticle, WeaponMeshComponent, TEXT("Muzzle"));
	}
}

void AWeaponBase::GetFireViewPoint(FVector& Position, FVector& Direction) const
{
	if (!MyMaster)
	{
		Position = GetActorLocation();
		Direction = GetActorForwardVector();
		return;
	}

	Position = MyMaster->GetPawnViewLocation();
	Direction = MyMaster->GetBaseAimRotation().Vector();

	if (APlayerController* PlayerController = Cast<APlayerController>(MyMaster->GetController()))
	{
		if (MyMaster->IsLocallyControlled())
		{
			FRotator ViewRotation;
			PlayerController->GetPlayerViewPoint(Position, ViewRotation);
			Direction = ViewRotation.Vector();
		}
	}
}

float AWeaponBase::GetReloadDuration() const
{
	if (!ReloadMontage)
	{
		return 1.0f;
	}

	const FName SectionName = MyMaster && MyMaster->IsIronSight()
		? TEXT("Default")
		: TEXT("IronSights");
	const int32 SectionIndex = ReloadMontage->GetSectionIndex(SectionName);
	if (SectionIndex != INDEX_NONE)
	{
		return FMath::Max(0.1f, ReloadMontage->GetSectionLength(SectionIndex));
	}
	return FMath::Max(0.1f, ReloadMontage->GetPlayLength());
}

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
	WeaponMeshComponent->AttachToComponent(
		MyMaster->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		TEXT("WeaponSocket"));

	if (HasAuthority())
	{
		SetOwner(MyMaster);
	}
}

void AWeaponBase::StartFire()
{
	if (!MyMaster || bWantsToFire
		|| CurrentState == EWeaponState::EWS_Reloading
		|| CurrentState == EWeaponState::EWS_Empty)
	{
		return;
	}

	bWantsToFire = true;
	RequestFireShot();
	if (bWantsToFire)
	{
		GetWorldTimerManager().SetTimer(
			FireTimerHandle,
			this,
			&ThisClass::RequestFireShot,
			FireInterval,
			true,
			FireInterval);
	}
}

void AWeaponBase::StopFire()
{
	bWantsToFire = false;
	GetWorldTimerManager().ClearTimer(FireTimerHandle);

	if (!HasAuthority())
	{
		Server_StopFire();
		return;
	}

	if (CurrentState == EWeaponState::EWS_Firing)
	{
		CurrentState = CurrClipVolume > 0
			? EWeaponState::EWS_Normal
			: EWeaponState::EWS_Empty;
		ForceNetUpdate();
	}
}

void AWeaponBase::ReloadClip()
{
	if (!MyMaster || CurrentState == EWeaponState::EWS_Reloading
		|| CurrClipVolume >= MaxClipVolume)
	{
		return;
	}

	bWantsToFire = false;
	GetWorldTimerManager().ClearTimer(FireTimerHandle);

	if (HasAuthority())
	{
		StartReloadOnServer();
		return;
	}

	CurrentState = EWeaponState::EWS_Reloading;
	if (ReloadMontage)
	{
		MyMaster->PlayAnimMontage(
			ReloadMontage,
			1.0f,
			MyMaster->IsIronSight() ? TEXT("Default") : TEXT("IronSights"));
	}
	Server_ReloadClip();
}

void AWeaponBase::StartReloadOnServer()
{
	if (!HasAuthority() || !MyMaster || CurrentState == EWeaponState::EWS_Reloading
		|| CurrClipVolume >= MaxClipVolume)
	{
		return;
	}

	PendingReloadAmount = MaxClipVolume - CurrClipVolume;
	const int32 AmmoItemId = GetAmmoItemId();
	if (AmmoItemId != INDEX_NONE)
	{
		UPackageComponent* Package = MyMaster->GetPackageComponent();
		if (!Package)
		{
			return;
		}

		PendingReloadAmount = FMath::Min(PendingReloadAmount, Package->GetItemQuantityById(AmmoItemId));
		if (PendingReloadAmount <= 0 || !Package->ConsumeItemById(AmmoItemId, PendingReloadAmount))
		{
			PendingReloadAmount = 0;
			return;
		}
	}

	StopFire();
	CurrentState = EWeaponState::EWS_Reloading;
	Multicast_PlayReloadMontage();
	GetWorldTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&ThisClass::MakeFullClip,
		GetReloadDuration(),
		false);
	ForceNetUpdate();
}

void AWeaponBase::MakeFullClip()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(ReloadTimerHandle);
	CurrClipVolume = FMath::Clamp(CurrClipVolume + PendingReloadAmount, 0, MaxClipVolume);
	PendingReloadAmount = 0;
	CurrentState = EWeaponState::EWS_Normal;

	if (OnWeaponClipChanged.IsBound())
	{
		OnWeaponClipChanged.Broadcast(CurrClipVolume, MaxClipVolume);
	}
	ForceNetUpdate();
}

int32 AWeaponBase::GetAmmoItemId() const
{
	const UWorld* World = GetWorld();
	UPropsSubsystem* Props = World ? World->GetGameInstance()->GetSubsystem<UPropsSubsystem>() : nullptr;
	return Props ? Props->GetAmmoItemIdForWeapon(ID) : INDEX_NONE;
}

int32 AWeaponBase::ExtractLoadedAmmo()
{
	if (!HasAuthority() || GetAmmoItemId() == INDEX_NONE)
	{
		return 0;
	}

	const int32 ExtractedAmmo = CurrClipVolume;
	CurrClipVolume = 0;
	PendingReloadAmount = 0;
	CurrentState = EWeaponState::EWS_Normal;
	ForceNetUpdate();
	return ExtractedAmmo;
}

void AWeaponBase::PrepareForSurvivalInventory()
{
	if (HasAuthority() && GetAmmoItemId() != INDEX_NONE)
	{
		CurrClipVolume = 0;
		PendingReloadAmount = 0;
		CurrentState = EWeaponState::EWS_Normal;
		ForceNetUpdate();
	}
}

void AWeaponBase::OnRep_CurrentClipVolume()
{
	if (CurrClipVolume <= 0)
	{
		bWantsToFire = false;
		GetWorldTimerManager().ClearTimer(FireTimerHandle);
	}
	if (OnWeaponClipChanged.IsBound())
	{
		OnWeaponClipChanged.Broadcast(CurrClipVolume, MaxClipVolume);
	}
}

void AWeaponBase::OnRep_ID()
{
	if (MyMaster && MyMaster->GetPackageComponent())
	{
		MyMaster->GetPackageComponent()->BroadcastCurrentEquipmentState();
	}
}

void AWeaponBase::OnRep_CurrentState()
{
	if (CurrentState == EWeaponState::EWS_Reloading
		|| CurrentState == EWeaponState::EWS_Empty)
	{
		bWantsToFire = false;
		GetWorldTimerManager().ClearTimer(FireTimerHandle);
	}
}

void AWeaponBase::Server_RequestFireShot_Implementation(
	FVector_NetQuantize ViewOrigin,
	FVector_NetQuantizeNormal ViewDirection)
{
	ProcessFireShot(ViewOrigin, ViewDirection);
}

void AWeaponBase::Server_StopFire_Implementation()
{
	StopFire();
}

void AWeaponBase::Server_ReloadClip_Implementation()
{
	StartReloadOnServer();
}

void AWeaponBase::Multicast_PlayFireEffect_Implementation()
{
	if (MyMaster && MyMaster->IsLocallyControlled() && !HasAuthority())
	{
		return;
	}
	SpawnEffect();
}

void AWeaponBase::Multicast_PlayReloadMontage_Implementation()
{
	if (GetNetMode() == NM_DedicatedServer || !ReloadMontage || !MyMaster)
	{
		return;
	}
	if (MyMaster->IsLocallyControlled() && !HasAuthority())
	{
		return;
	}

	MyMaster->PlayAnimMontage(
		ReloadMontage,
		1.0f,
		MyMaster->IsIronSight() ? TEXT("Default") : TEXT("IronSights"));
}
