// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
	UPROPERTY(EditAnywhere)
	FText Name;
	UPROPERTY(EditAnywhere)
	UTexture2D* Icon;
	
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
};

UCLASS()
class LEGOGAME_API UPropsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	
	const FPropsBase* GetPropsById(const int32& ID);

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
protected:
	
	UPROPERTY()
	TObjectPtr<UDataTable> SkinDataTable;
	
	UPROPERTY()
	TObjectPtr<UDataTable> WeaponDataTable;
	
	TMap<int32, FPropsBase*> PropsMap;
};
