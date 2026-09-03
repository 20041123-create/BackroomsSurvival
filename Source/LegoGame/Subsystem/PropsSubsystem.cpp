// Fill out your copyright notice in the Description page of Project Settings.


#include "PropsSubsystem.h"

#include "Evaluation/MovieSceneEvaluationState.h"
#include "Evaluation/SequenceDirectorPlaybackCapability.h"
#include "LegoGame/LegoGame.h"
#include "LegoGame/Survival/Contracts/SurvivalGameplayTags.h"
#include "LegoGame/Weapon/WeaponBase.h"


void UPropsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	//加载表格数据
	SkinDataTable = LoadObject<UDataTable>(this,TEXT("/Script/Engine.DataTable'/Game/LegoGame/Data/Props/DT_Skin.DT_Skin'"));
	//处理表格，将数据表格转存到TMap中
	if (SkinDataTable)
	{
		for (auto It : SkinDataTable->GetRowMap())
		{
			int32 ID = 0;
			FString Left;
			FString Right;
			if (It.Key.ToString().Split(TEXT("_"),&Left,&Right))
			{
				ID = FCString::Atoi(*Right)+1;
			}
			//UE_LOG(LogTemp, Warning, TEXT("%d"),ID);
			//转存到TMap
			if (!PropsMap.Contains(ID))
			{
				PropsMap.Add(ID,reinterpret_cast<FPropsBase*>(It.Value));
			}
		}
	}
	
	WeaponDataTable = LoadObject<UDataTable>(this,TEXT("/Script/Engine.DataTable'/Game/LegoGame/Data/Props/DT_Weapon.DT_Weapon'"));
	if (WeaponDataTable)
	{
		for (auto It : WeaponDataTable->GetRowMap())
		{
			int32 ID = WEAPON_INDEX;
			FString Left;
			FString Right;
			if (It.Key.ToString().Split(TEXT("_"),&Left,&Right))
			{
				ID = FCString::Atoi(*Right)+1+WEAPON_INDEX;
				
			}
			//UE_LOG(LogTemp, Warning, TEXT("%d"),ID);
			//转存到TMap
			if (!PropsMap.Contains(ID))
			{
				PropsMap.Add(ID,reinterpret_cast<FPropsBase*>(It.Value));
			}
		}
	}
}

const FPropsBase* UPropsSubsystem::GetPropsById(const int32& ID)
{
	if (PropsMap.Contains(ID))
	{
		return PropsMap[ID];
	}
	return nullptr;
}

bool UPropsSubsystem::GetSurvivalItemView(int32 ItemId, FSurvivalItemView& OutItem) const
{
	const FPropsBase* Props = PropsMap.FindRef(ItemId);
	if (!Props)
	{
		return false;
	}

	OutItem.Stack.ItemId = ItemId;
	OutItem.Stack.SlotId = INDEX_NONE;
	OutItem.Stack.Quantity = 1;
	OutItem.DisplayName = Props->Name;
	OutItem.Icon = Props->Icon;
	OutItem.ItemTags = Props->SurvivalItemTags;
	OutItem.MaxStackSize = FMath::Max(1, Props->MaxStackSize);
	return true;
}

void UPropsSubsystem::GetSurvivalItemIdsByTag(FGameplayTag ItemTag, TArray<int32>& OutItemIds) const
{
	OutItemIds.Reset();
	if (!ItemTag.IsValid())
	{
		return;
	}

	for (const TPair<int32, FPropsBase*>& Pair : PropsMap)
	{
		FSurvivalItemView ItemView;
		if (GetSurvivalItemView(Pair.Key, ItemView) && ItemView.ItemTags.HasTag(ItemTag))
		{
			OutItemIds.Add(Pair.Key);
		}
	}

	OutItemIds.Sort();
}

int32 UPropsSubsystem::GetMaxStackSize(int32 ItemId) const
{
	const FPropsBase* Props = PropsMap.FindRef(ItemId);
	return Props ? FMath::Max(1, Props->MaxStackSize) : 0;
}

bool UPropsSubsystem::GetConsumableEffects(int32 ItemId, float& OutHealth, float& OutHunger, float& OutThirst) const
{
	OutHealth = 0.0f;
	OutHunger = 0.0f;
	OutThirst = 0.0f;

	const FPropsBase* Props = PropsMap.FindRef(ItemId);
	if (!Props)
	{
		return false;
	}

	OutHealth = Props->HealthRestore;
	OutHunger = Props->HungerRestore;
	OutThirst = Props->ThirstRestore;
	return !FMath::IsNearlyZero(OutHealth)
		|| !FMath::IsNearlyZero(OutHunger)
		|| !FMath::IsNearlyZero(OutThirst);
}

int32 UPropsSubsystem::GetAmmoItemIdForWeapon(int32 WeaponItemId) const
{
	const FPropsBase* Props = PropsMap.FindRef(WeaponItemId);
	if (!Props || Props->Type != EPropsType::EPT_Weapon)
	{
		return INDEX_NONE;
	}

	return static_cast<const FWeaponBaseHeader*>(Props)->AmmoItemId;
}

bool UPropsSubsystem::IsSurvivalWeaponItem(int32 ItemId) const
{
	const FPropsBase* Props = PropsMap.FindRef(ItemId);
	return Props && Props->Type == EPropsType::EPT_Weapon
		&& Props->SurvivalItemTags.HasTagExact(LG::SurvivalTags::Item_Weapon);
}

bool UPropsSubsystem::IsStackableSurvivalResourceItem(int32 ItemId) const
{
	const FPropsBase* Props = PropsMap.FindRef(ItemId);
	return Props && !Props->SurvivalItemTags.IsEmpty() && Props->Type != EPropsType::EPT_Weapon;
}

bool UPropsSubsystem::IsValidSurvivalWeaponDefinition(int32 ItemId) const
{
	if (!IsSurvivalWeaponItem(ItemId))
	{
		return false;
	}

	const FWeaponBaseHeader* Weapon = static_cast<const FWeaponBaseHeader*>(PropsMap.FindRef(ItemId));
	if (!Weapon || !Weapon->WeaponClass || !Weapon->SkeletalMesh || !Weapon->Icon || Weapon->AmmoItemId == INDEX_NONE)
	{
		return false;
	}

	const FPropsBase* Ammo = PropsMap.FindRef(Weapon->AmmoItemId);
	return Ammo && Ammo->SurvivalItemTags.HasTagExact(LG::SurvivalTags::Item_Ammo);
}
