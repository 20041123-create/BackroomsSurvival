// Fill out your copyright notice in the Description page of Project Settings.


#include "HallGameState.h"

#include "HallHUD.h"
#include "LegoGame/LegoGame.h"
#include "LegoGame/GamePlay/GameMenu/Hall/HallUserWidget.h"


void AHallGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);
	if (OnAddPlayerState.IsBound())
	{
		OnAddPlayerState.Broadcast(PlayerState);
	}
}

void AHallGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);
	if (OnRemovePlayerState.IsBound())
	{
		OnRemovePlayerState.Broadcast(PlayerState);
	}
}

void AHallGameState::PushPublicMessage(EChatChannel Channel, const FText& Text)
{
	Multi_PushPublicMessage(Channel,Text);
	
}

void AHallGameState::Multi_PushPublicMessage_Implementation(EChatChannel Channel, const FText& Text)
{
	//将消息显示在终端上
	if (AHallHUD* HUD = Cast<AHallHUD>(GetWorld()->GetFirstPlayerController()->GetHUD()))
	{
		HUD->GetHallUserWidget()->PushMessage(Channel,Text);
	}
	
}
