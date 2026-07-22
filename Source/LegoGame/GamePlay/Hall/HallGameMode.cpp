// Fill out your copyright notice in the Description page of Project Settings.


#include "HallGameMode.h"

#include "HallGameState.h"
#include "HallHUD.h"
#include "HallPlayerController.h"
#include "HallPlayerState.h"
#include "GameFramework/GameSession.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LegoGame/LegoGame.h"
#include "LegoGame/GamePlay/LgGameInstance.h"
#include "LegoGame/GamePlay/GameMenu/Hall/HallUserWidget.h"


AHallGameMode::AHallGameMode()
{
	HUDClass = AHallHUD::StaticClass();
	GameStateClass = AHallGameState::StaticClass();
	PlayerStateClass = AHallPlayerState::StaticClass();
	PlayerControllerClass = AHallPlayerController::StaticClass();
	bUseSeamlessTravel = true;
}

void AHallGameMode::KickPlayer(APlayerController* PlayerController)
{
	//剔除玩家
	if (PlayerController)
	{
		GameSession->KickPlayer(PlayerController,NSLOCTEXT("ui","scpiop1","你已被房主移除！"));
	}
	
}

void AHallGameMode::EndLgGame()
{
	//通知游戏内所有玩家退出游戏
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Pc = It->Get();
		if (Pc)
		{
			//让所有玩家漫游到游戏主页面
			Pc->ClientReturnToMainMenuWithTextReason(NSLOCTEXT("ui","vlogios1","房间已被解散！"));
		}
	}
	
}

void AHallGameMode::StartLgGame()
{
	//检查是否所有玩家准备好了
	bool bAllReady = true;
	for (auto It : GetGameState<AHallGameState>()->PlayerArray)
	{
		if (AHallPlayerState* Ps = Cast<AHallPlayerState>(It))
		{
			if (!Ps->IsReady() && !Ps->IsMaster())
			{
				bAllReady = false;
				break;
			}
		}
	}
	
	if (!bAllReady)
	{
		if (AHallHUD* HallHUD = Cast<AHallHUD>(GetWorld()->GetFirstPlayerController()->GetHUD()))
		{
			HallHUD->GetHallUserWidget()->PushMessage(EChatChannel::ECC_System, NSLOCTEXT("ui","totqit1","当前有玩家未准备，无法开始游戏！"));
		}
	}
	else
	{
		
		//切换关卡
		if (ULgGameInstance* Gi = Cast<ULgGameInstance>(GetWorld()->GetGameInstance()))
		{
			FString MapPath = Gi->GetMapName() == TEXT("") ? TEXT("/Game/LegoGame/Maps/TestMap") : Gi->GetMapName();
			//切换地图
			GetWorld()->ServerTravel(MapPath);
			//UKismetSystemLibrary::ExecuteConsoleCommand(this,FString::Printf(TEXT("ServerTravel %s?listen"),*MapPath));
		}
	}
}
