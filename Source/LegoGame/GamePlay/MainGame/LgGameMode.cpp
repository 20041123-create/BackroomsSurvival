// Fill out your copyright notice in the Description page of Project Settings.


#include "LgGameMode.h"

#include "LgHUD.h"
#include "LgPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "LegoGame/Player/LgPlayerController.h"
#include "LegoGame/Player/PlayerCharacter.h"
#include "LegoGame/Scene/LgPlayerStart.h"
#include "LegoGame/Subsystem/PropsSubsystem.h"

ALgGameMode::ALgGameMode()
{
	//设置默认角色类
	ConstructorHelpers::FClassFinder<APlayerCharacter> PlayerClass(TEXT("/Script/Engine.Blueprint'/Game/LegoGame/Blueprints/Player/BP_Player.BP_Player_C'"));
	DefaultPawnClass = PlayerClass.Class;
	
	PlayerControllerClass = ALgPlayerController::StaticClass();
	HUDClass = ALgHUD::StaticClass();
	PlayerStateClass = ALgPlayerState::StaticClass();
}

APawn* ALgGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	//调整出生位置
	FTransform NewSpawnTransform = SpawnTransform;
	if (ALgPlayerState* PlayerState = NewPlayer ? NewPlayer->GetPlayerState<ALgPlayerState>() : nullptr)
	{
		GetTeamSpawnTransform(PlayerState->GetTeamType(), NewSpawnTransform);
		NewPlayer->SetControlRotation(NewSpawnTransform.Rotator());
	}
	
	return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, NewSpawnTransform);
}

void ALgGameMode::GetTeamSpawnTransform(ETeamType TeamType, FTransform& SpawnTransform)
{
	if (PlayerStartMap.Num() == 0)
	{
		//全局搜索一下
		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(this,ALgPlayerStart::StaticClass(),Actors);
		
		for (AActor* Actor : Actors)
		{
			if (ALgPlayerStart* PlayerStart = Cast<ALgPlayerStart>(Actor))
			{
				if (!PlayerStartMap.Contains(PlayerStart->GetTeamType()))
				{
					PlayerStartMap.Add(PlayerStart->GetTeamType(), PlayerStart);
				}
			}
		}
	}
	
	if (PlayerStartMap.Contains(TeamType))
	{
		SpawnTransform = PlayerStartMap[TeamType]->GetTransform();
	}
	
}

// void ALgGameMode::TestFun(int32 N)
// {
// 	if (N==0)
// 	{
// 		//向子系统写入数据
// 		//获取子系统（单例对象
// 		GetWorld()->GetGameInstance()->GetSubsystem<UPropsSubsystem>()->Number = 109;
// 	}
// 	else
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("%d"),GetWorld()->GetGameInstance()->GetSubsystem<UPropsSubsystem>()->Number);
// 	}
// }
