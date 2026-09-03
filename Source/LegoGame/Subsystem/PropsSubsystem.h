// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LegoGame/Survival/Contracts/SurvivalTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PropsSubsystem.generated.h"

//记录道具的数据信息
//装饰性道具，武器型道具
/**
 * 
 */

class AWeaponBase;

UENUM()
enum class EPropsType : uint8
{
	EPT_Skin,
	EPT_Weapon,
};
//创建表头
USTRUCT()
struct FPropsBase : public FTableRowBase
{
	GENERATED_BODY()

	FPropsBase()
		: Type(EPropsType::EPT_Skin)
	{
	}

	UPROPERTY(EditAnywhere)
	FText Name;
	UPROPERTY(EditAnywhere)
	UTexture2D* Icon;

	// Survival metadata intentionally lives alongside the legacy prop rows so
	// existing weapon and skin assets remain valid with their default values.
	UPROPERTY(EditAnywhere, Category="Survival")
	FGameplayTagContainer SurvivalItemTags;

	UPROPERTY(EditAnywhere, Category="Survival", meta=(ClampMin="1"))
	int32 MaxStackSize = 1;

	UPROPERTY(EditAnywhere, Category="Survival")
	float HealthRestore = 0.0f;

	UPROPERTY(EditAnywhere, Category="Survival")
	float HungerRestore = 0.0f;

	UPROPERTY(EditAnywhere, Category="Survival")
	float ThirstRestore = 0.0f;
	
	EPropsType Type;
};

UENUM()
enum class ESkinType : uint8
{
	EST_None,
	EST_Cap,
	EST_Glasses,
	EST_Helmet,
	EST_Hair,
	EST_Masker,
	EST_Beard,
	EST_Clothes,
	EST_Package,
};

USTRUCT()
struct FSkinHeader : public FPropsBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere)
	ESkinType SkinType;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UStaticMesh> StaticMesh;
	UPROPERTY(EditAnywhere)
	TObjectPtr<USkeletalMesh> SkeletalMesh;
};

USTRUCT()
struct FWeaponBaseHeader : public FPropsBase
{
	GENERATED_BODY()
	
	FWeaponBaseHeader(){Type = EPropsType::EPT_Weapon;}
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<USkeletalMesh> SkeletalMesh;
	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeaponBase> WeaponClass;

	UPROPERTY(EditAnywhere, Category="Survival", meta=(ClampMin="-1"))
	int32 AmmoItemId = INDEX_NONE;
};

UCLASS()
class LEGOGAME_API UPropsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	
	const FPropsBase* GetPropsById(const int32& ID);
	bool GetSurvivalItemView(int32 ItemId, FSurvivalItemView& OutItem) const;
	/** Resolves matching Survival item definitions in stable ascending ItemId order. */
	void GetSurvivalItemIdsByTag(FGameplayTag ItemTag, TArray<int32>& OutItemIds) const;
	int32 GetMaxStackSize(int32 ItemId) const;
	bool GetConsumableEffects(int32 ItemId, float& OutHealth, float& OutHunger, float& OutThirst) const;
	int32 GetAmmoItemIdForWeapon(int32 WeaponItemId) const;
	/** A production Survival weapon is explicitly classified by both its legacy type and formal Gameplay Tag. */
	bool IsSurvivalWeaponItem(int32 ItemId) const;
	/** Stackable inventory excludes every legacy weapon, including tagged weapons resolved by the runtime provider. */
	bool IsStackableSurvivalResourceItem(int32 ItemId) const;
	/** Validates the assets and ammo route required to create an equipable Survival weapon. */
	bool IsValidSurvivalWeaponDefinition(int32 ItemId) const;

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
protected:
	
	UPROPERTY()
	TObjectPtr<UDataTable> SkinDataTable;
	
	UPROPERTY()
	TObjectPtr<UDataTable> WeaponDataTable;
	
	TMap<int32, FPropsBase*> PropsMap;
};
