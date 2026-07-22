// Copyright Epic Games, Inc. All Rights Reserved.

#include "LegoGame.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, LegoGame, "LegoGame" );

FText LG::GetJobText(EJobType JobType)
{
	FText t1 = NSLOCTEXT("ui","cdd2","NONE");
	switch (JobType)
	{
	case EJobType::EJT_00:
		t1 = NSLOCTEXT("ui","1cdd2","步兵");
		break;
	case EJobType::EJT_01:
		t1 = NSLOCTEXT("ui","2cdd2","医护兵");
		break;
	case EJobType::EJT_02:
		t1 = NSLOCTEXT("ui","3cdd2","狙击手");
		break;
	case EJobType::EJT_03:
		t1 = NSLOCTEXT("ui","4cdd2","生化兵");
		break;
	}
	return t1;
}

FLinearColor LG::GetChatChannelColor(EChatChannel ChatChannelType)
{
	switch (ChatChannelType)
	{
	case EChatChannel::ECC_World:
		return FLinearColor::Blue;
	case EChatChannel::ECC_Team:
		return FLinearColor::Green;
	case EChatChannel::ECC_Personal:
		return FLinearColor::Red;
	case EChatChannel::ECC_System:
		return FLinearColor::Yellow;
		default:
		return FLinearColor::White;
	}
	
}

FText LG::GetChatChannelText(EChatChannel ChatChannelType)
{
	switch (ChatChannelType)
	{
	case EChatChannel::ECC_World:
		return NSLOCTEXT("ui","ci5dv","世界");
	case EChatChannel::ECC_Team:
		return NSLOCTEXT("ui","1ci5dv","队伍");
	case EChatChannel::ECC_Personal:
		return NSLOCTEXT("ui","2ci5dv","用户");
	case EChatChannel::ECC_System:
		return NSLOCTEXT("ui","3ci5dv","系统");
	default:
		return NSLOCTEXT("ui","4ci5dv","无");
	}
	
}

const TCHAR* LG::GetJobMeshSource(ETeamType TeamType, EJobType JobType)
{
	if (TeamType == ETeamType::ETT_Police)
	{
		switch (JobType)
		{
			case EJobType::EJT_00:
			return TEXT("/Script/Engine.SkeletalMesh'/Game/Polygon_BattleRoyale/Meshes/Characters/SK_Chr_BusinessMale_01.SK_Chr_BusinessMale_01'");
			case EJobType::EJT_01:
			return TEXT("/Script/Engine.SkeletalMesh'/Game/Polygon_BattleRoyale/Meshes/Characters/SK_Chr_MilitaryMale_01.SK_Chr_MilitaryMale_01'");
			case EJobType::EJT_02:
			return TEXT("/Script/Engine.SkeletalMesh'/Game/Polygon_BattleRoyale/Meshes/Characters/SK_Chr_ToplessMale_01.SK_Chr_ToplessMale_01'");
			case EJobType::EJT_03:
			return TEXT("/Script/Engine.SkeletalMesh'/Game/Polygon_BattleRoyale/Meshes/Characters/SK_Chr_GhillieSuit_01.SK_Chr_GhillieSuit_01'");
		}
	}
	else if (TeamType == ETeamType::ETT_Bandit)
	{
		switch (JobType)
		{
		case EJobType::EJT_00:
			return TEXT("/Script/Engine.SkeletalMesh'/Game/Polygon_BattleRoyale/Meshes/Characters/SK_Chr_RedneckMale_01.SK_Chr_RedneckMale_01'");
		case EJobType::EJT_01:
			return TEXT("/Script/Engine.SkeletalMesh'/Game/Polygon_BattleRoyale/Meshes/Characters/SK_Chr_SportyMale_01.SK_Chr_SportyMale_01'");
		case EJobType::EJT_02:
			return TEXT("/Script/Engine.SkeletalMesh'/Game/Polygon_BattleRoyale/Meshes/Characters/SK_Chr_SportyMale_02.SK_Chr_SportyMale_02'");
		case EJobType::EJT_03:
			return TEXT("/Script/Engine.SkeletalMesh'/Game/Polygon_BattleRoyale/Meshes/Characters/SK_Chr_SportyFemale_01.SK_Chr_SportyFemale_01'");
		}
	}
	
	return TEXT("");
	
}
