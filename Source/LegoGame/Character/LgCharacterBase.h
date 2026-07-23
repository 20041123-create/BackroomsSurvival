// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagAssetInterface.h"
#include "GameFramework/Character.h"
#include "LegoGame/Interface/SkinInterface.h"
#include "LgCharacterBase.generated.h"

enum class ETeamType : uint8;
class ALgCharacterBase;
class AWeaponBase;
class USkinComponent;
class UPackageComponent;
class APlayerCharacter;
class UBillboardComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(NotifyDamage,ALgCharacterBase*)


UCLASS()
class LEGOGAME_API ALgCharacterBase : public ACharacter, public IGameplayTagAssetInterface,public ISkinInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ALgCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());//对象属性构建器，FObjectInitializer::Get()默认值避免子类重写带参构造


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& OutTagContainer) const override;
	
	virtual USkeletalMeshComponent* GetSkeletalMeshComponent() override {return GetMesh();}

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	bool IsSprinting()const;
	
	bool IsCrouched()const{return bIsCrouched;}//bIsCrouched 系统定义
	
	bool IsJumping()const;
	
	bool IsIronSight()const{return bIronSight;}
	
	TObjectPtr<UPackageComponent> GetPackageComponent()const {return PackageComponent;}
	
	TObjectPtr<USkinComponent> GetSkinComponent()const {return SkinComponent;}
	
	TObjectPtr<AWeaponBase> GetHoldWeapon()const;
	
	ETeamType GetTeamType() const;
	
	void StartSprint();
	void StopSprint();
	
	void StartFire();
	void StopFire();
	
	void StartIronSight();
	void StopIronSight();
	
	void UpdateJobMesh();
	
protected:
	
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	
	virtual void OnConstruction(const FTransform& Transform) override;
	
	
	void DoCrouch();
	
	void OnEquipWeapon(int32 ID);
	void OnUnEquipWeapon(int32 ID);
	
	void ReloadWeapon();
	
	//RPC
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetSprint(bool bNewSprint);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetIronSight(bool bNewIronSight);
	
protected:
	UPROPERTY(Replicated)
	bool bSprinting;
	UPROPERTY(Replicated)
	bool bIronSight;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBillboardComponent> BillboardComponent; 
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)
	TObjectPtr<UPackageComponent> PackageComponent;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkinComponent> SkinComponent;
	
	UPROPERTY(EditAnywhere)
	ETeamType TeamType;
	
	UPROPERTY(EditAnywhere)
	FGameplayTagContainer TagContainer;
	
	//int32 UpdateMeshCount;
	
public:
	
	NotifyDamage OnReceiveDamage;
};
