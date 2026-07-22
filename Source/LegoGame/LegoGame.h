// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#define CUSTOM_KEY_SLOT TEXT("CustomSlot")

#define WEAPON_INDEX 100


#define WEAPON_TRACE ECC_GameTraceChannel2

//声明阵营关系枚举
UENUM(BlueprintType)
enum class ETeamType : uint8
{
	ETT_None,
	ETT_Police,
	ETT_Bandit
};

UENUM(BlueprintType)
enum class EJobType : uint8
{
	EJT_None,
	EJT_00,
	EJT_01,
	EJT_02,
	EJT_03,
};

UENUM(BlueprintType)
enum class EChatChannel : uint8
{
	ECC_World,
	ECC_Team,
	ECC_Personal,
	ECC_System,
};

namespace LG
{
	FText GetJobText(EJobType JobType);
	
	FLinearColor GetChatChannelColor(EChatChannel ChatChannelType);
	FText GetChatChannelText(EChatChannel ChatChannelType);
	
	const TCHAR* GetJobMeshSource(ETeamType TeamType, EJobType JobType);
}
