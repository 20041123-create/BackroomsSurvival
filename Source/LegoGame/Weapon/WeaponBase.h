// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(WeaponClipChanged,int32,int32);


class ALgCharacterBase;
class UParticleSystem;
class USoundCue;
class USkeletalMeshComponent;

UENUM()
enum class EWeaponState : uint8
{
	EWS_Normal,
	EWS_Reloading,
	EWS_Firing,
	EWS_Empty,
};



UCLASS()
class LEGOGAME_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AWeaponBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	//生成子弹
	void RequestFireShot();
	void ProcessFireShot(const FVector& ViewOrigin, const FVector& ViewDirection);
	
	void SpawnDamage(const FVector& ViewOrigin, const FVector& ViewDirection);
	//发射特效
	void SpawnEffect();
	
	void GetFireViewPoint(FVector& Position, FVector& Direction) const;
	float GetReloadDuration() const;
	void StartReloadOnServer();

	UFUNCTION()
	void OnRep_CurrentClipVolume();

	UFUNCTION()
	void OnRep_ID();

	UFUNCTION()
	void OnRep_CurrentState();

	UFUNCTION(Server, Unreliable)
	void Server_RequestFireShot(FVector_NetQuantize ViewOrigin, FVector_NetQuantizeNormal ViewDirection);

	UFUNCTION(Server, Reliable)
	void Server_StopFire();

	UFUNCTION(Server, Reliable)
	void Server_ReloadClip();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayFireEffect();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayReloadMontage();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	void SetID(int32 NewID){ID = NewID;}
	int32 GetID() const {return ID;}
	
	void SetMaster(ALgCharacterBase* InMaster);
	
	void StartFire();
	void StopFire();
	//换弹动画
	void ReloadClip();
	//回填子弹
	void MakeFullClip();
	
	int32 GetCurrentClipVolume() const {return CurrClipVolume;}
	int32 GetMaxClipVolume() const {return MaxClipVolume;}
	float GetDamagePerShot() const { return DamagePerShot; }
	int32 GetAmmoItemId() const;
	int32 ExtractLoadedAmmo();
	void PrepareForSurvivalInventory();
	
	WeaponClipChanged OnWeaponClipChanged;
	
	EWeaponState GetCurrentState() const {return CurrentState;}
	
protected:
	UPROPERTY(ReplicatedUsing=OnRep_ID)
	int32 ID;
	//最大子弹容量
	UPROPERTY(EditAnywhere)
	int32 MaxClipVolume;
	//当前弹夹容积
	UPROPERTY(ReplicatedUsing=OnRep_CurrentClipVolume)
	int32 CurrClipVolume;
	//开火间隔
	UPROPERTY(EditAnywhere)
	float FireInterval;
	//开火特效
	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> FireParticle;
	//开火音效
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> FireSound;
	//武器骨骼体
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> WeaponMeshComponent;
	//换弹蒙太奇
	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> ReloadMontage;
	//射击距离
	UPROPERTY(EditAnywhere)
	float ShotDistance;

	/** Server-authoritative damage applied by one confirmed weapon trace. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Damage", meta=(ClampMin="0.0"))
	float DamagePerShot = 10.0f;
	
	UPROPERTY(Replicated)
	TObjectPtr<ALgCharacterBase> MyMaster;
	
	FTimerHandle FireTimerHandle;
	FTimerHandle ReloadTimerHandle;
	
	float LastFireTime;
	int32 PendingReloadAmount = 0;
	
	UPROPERTY(ReplicatedUsing=OnRep_CurrentState)
	EWeaponState CurrentState;

	bool bWantsToFire = false;

	UPROPERTY(EditDefaultsOnly, Category="Network", meta=(ClampMin="0.0"))
	float MaxClientViewOriginError = 600.0f;

	UPROPERTY(EditDefaultsOnly, Category="Network", meta=(ClampMin="0.0", ClampMax="180.0"))
	float MaxClientAimErrorDegrees = 30.0f;
	
};
