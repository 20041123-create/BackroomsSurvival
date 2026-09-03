#include "HallGameMode.h"

#include "HallGameState.h"
#include "HallHUD.h"
#include "HallPlayerController.h"
#include "HallPlayerState.h"
#include "GameFramework/GameSession.h"
#include "LegoGame/LegoGame.h"
#include "LegoGame/GamePlay/LgGameInstance.h"

namespace
{
	constexpr TCHAR LegacyTestMapPath[] = TEXT("/Game/LegoGame/Maps/TestMap");
	constexpr TCHAR SurvivalMapPath[] = TEXT("/Game/LegoGame/Survival/Maps/L_SurvivalWorld");
}

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
	if (PlayerController && GameSession)
	{
		GameSession->KickPlayer(
			PlayerController,
			NSLOCTEXT("ui", "scpiop1", "你已被房主移除！"));
	}
}

void AHallGameMode::EndLgGame()
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		if (APlayerController* PlayerController = Iterator->Get())
		{
			PlayerController->ClientReturnToMainMenuWithTextReason(
				NSLOCTEXT("ui", "vlogios1", "房间已被解散！"));
		}
	}
}

void AHallGameMode::StartLgGame()
{
	AHallGameState* HallGameState = GetGameState<AHallGameState>();
	if (!HallGameState)
	{
		return;
	}

	bool bAllReady = true;
	for (APlayerState* PlayerState : HallGameState->PlayerArray)
	{
		if (const AHallPlayerState* HallPlayerState = Cast<AHallPlayerState>(PlayerState))
		{
			if (!HallPlayerState->IsReady() && !HallPlayerState->IsMaster())
			{
				bAllReady = false;
				break;
			}
		}
	}

	if (!bAllReady)
	{
		HallGameState->PushPublicMessage(
			EChatChannel::ECC_System,
			NSLOCTEXT("ui", "totqit1", "当前有玩家未准备，无法开始游戏！"));
		return;
	}

	if (ULgGameInstance* GameInstance = Cast<ULgGameInstance>(GetGameInstance()))
	{
		FString MapPath = GameInstance->GetMapName();
		if (MapPath.IsEmpty() || MapPath == LegacyTestMapPath)
		{
			MapPath = SurvivalMapPath;
		}
		GetWorld()->ServerTravel(MapPath);
	}
}
