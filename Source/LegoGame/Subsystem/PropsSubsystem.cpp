// Fill out your copyright notice in the Description page of Project Settings.


#include "PropsSubsystem.h"

#include "Evaluation/MovieSceneEvaluationState.h"
#include "Evaluation/SequenceDirectorPlaybackCapability.h"
#include "LegoGame/LegoGame.h"


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
