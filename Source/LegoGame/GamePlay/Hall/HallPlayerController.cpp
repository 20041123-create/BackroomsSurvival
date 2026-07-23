// Fill out your copyright notice in the Description page of Project Settings.


#include "HallPlayerController.h"

#include "HallGameState.h"
#include "HallPlayerState.h"
#include "HallGameMode.h"
#include "LegoGame/LegoGame.h"
#include "LegoGame/GamePlay/LgGameInstance.h"

void AHallPlayerController::SendChatMessage(EChatChannel Channel, const FText& Text)
{
	if (!HasAuthority())
	{
		Server_SendChatMessage(Channel, Text);
		return;
	}
	//处理消息
	AHallPlayerState* SenderPs = GetPlayerState<AHallPlayerState>();
	if (!SenderPs)
	{
		return;
	}
	if (Channel == EChatChannel::ECC_World)
	{
		if (AHallGameState* Gs = Cast<AHallGameState>(GetWorld()->GetGameState()))
		{
			Gs->PushPublicMessage(EChatChannel::ECC_World, FText::Format(NSLOCTEXT("ui","vgki13","{0}:{1}"),FText::FromString(SenderPs->GetHallPlayerName()),Text));
		}
	}
	else if (Channel == EChatChannel::ECC_Team)
	{
		for (auto Ps : GetWorld()->GetGameState()->PlayerArray)
		{
			if (AHallPlayerState* HallPs = Cast<AHallPlayerState>(Ps))
			{
				if (HallPs->GetTeamType() == SenderPs->GetTeamType())//说明同一个队伍
				{
					HallPs->SendChatMessageToSelf(EChatChannel::ECC_Team,FText::Format(NSLOCTEXT("ui","vgki13","{0}:{1}"),FText::FromString(SenderPs->GetHallPlayerName()),Text));
				}
			}
		}
	}
	else if (Channel == EChatChannel::ECC_Personal)
	{
		FString MessageString = Text.ToString();
		FString Left;
		FString Right;
		
		if (MessageString.Split(TEXT(" "),&Left,&Right))
		{
			for (auto Ps : GetWorld()->GetGameState()->PlayerArray)
			{
				if (AHallPlayerState* HallPs = Cast<AHallPlayerState>(Ps))
				{
					if (HallPs->GetHallPlayerName() == Left)
					{
						HallPs->SendChatMessageToSelf(EChatChannel::ECC_Personal,FText::Format(NSLOCTEXT("ui","ffkfkw1","{0}对你说：{1}"),
						FText::FromString(SenderPs->GetHallPlayerName()),FText::FromString(Right)));
					}
				}
			}
			SenderPs->SendChatMessageToSelf(EChatChannel::ECC_Personal,FText::Format(NSLOCTEXT("ui","cksdf1a","你对{0}说：{1}"),FText::FromString(Left),
				FText::FromString(Right)));
		}
		
	}
}

void AHallPlayerController::RequestStartGame()
{
	Server_RequestStartGame();
}

void AHallPlayerController::RequestEndGame()
{
	Server_RequestEndGame();
}

void AHallPlayerController::RequestKickPlayer(AHallPlayerState* TargetPlayerState)
{
	if (IsValid(TargetPlayerState))
	{
		Server_RequestKickPlayer(TargetPlayerState);
	}
}

void AHallPlayerController::ClientWasKicked_Implementation(const FText& KickReason)
{
	Super::ClientWasKicked_Implementation(KickReason);
	//记录错误消息到GameInstance
	if (ULgGameInstance* GameInstance = Cast<ULgGameInstance>(GetWorld()->GetGameInstance()))
	{
		GameInstance->SetErrorMessage(KickReason); 
	}
	
}

void AHallPlayerController::ClientReturnToMainMenuWithTextReason_Implementation(const FText& ReturnReason)
{
	Super::ClientReturnToMainMenuWithTextReason_Implementation(ReturnReason);
	if (ULgGameInstance* GameInstance = Cast<ULgGameInstance>(GetWorld()->GetGameInstance()))
	{
		GameInstance->SetErrorMessage(ReturnReason); 
	}
	
}

void AHallPlayerController::Server_SendChatMessage_Implementation(EChatChannel Channel, const FText& Text)
{
	SendChatMessage(Channel, Text);
	
}

bool AHallPlayerController::Server_SendChatMessage_Validate(EChatChannel Channel, const FText& Text)
{
	return true;
}

void AHallPlayerController::Server_RequestStartGame_Implementation()
{
	const AHallPlayerState* Requester = GetPlayerState<AHallPlayerState>();
	if (Requester && Requester->IsMaster())
	{
		if (AHallGameMode* GameMode = GetWorld()->GetAuthGameMode<AHallGameMode>())
		{
			GameMode->StartLgGame();
		}
	}
}

void AHallPlayerController::Server_RequestEndGame_Implementation()
{
	const AHallPlayerState* Requester = GetPlayerState<AHallPlayerState>();
	if (Requester && Requester->IsMaster())
	{
		if (AHallGameMode* GameMode = GetWorld()->GetAuthGameMode<AHallGameMode>())
		{
			GameMode->EndLgGame();
		}
	}
}

void AHallPlayerController::Server_RequestKickPlayer_Implementation(AHallPlayerState* TargetPlayerState)
{
	const AHallPlayerState* Requester = GetPlayerState<AHallPlayerState>();
	if (!Requester || !Requester->IsMaster() || !IsValid(TargetPlayerState)
		|| TargetPlayerState->IsMaster())
	{
		return;
	}

	if (AHallGameMode* GameMode = GetWorld()->GetAuthGameMode<AHallGameMode>())
	{
		GameMode->KickPlayer(Cast<APlayerController>(TargetPlayerState->GetOwningController()));
	}
}
