// Fill out your copyright notice in the Description page of Project Settings.


#include "HallPlayerState.h"

#include "HallHUD.h"
#include "LegoGame/LegoGame.h"
#include "LegoGame/GamePlay/GameMenu/Hall/HallUserWidget.h"
#include "LegoGame/GamePlay/MainGame/LgPlayerState.h"
#include "Net/UnrealNetwork.h"

void AHallPlayerState::SetPlayerInfo(int32 InHeadIndex, const FString& InHallPlayerName, ETeamType InTeamType,
                                     EJobType InJobType)
{
	if (!HasAuthority())
	{
		Server_SetPlayerInfo(InHeadIndex,InHallPlayerName,InTeamType,InJobType);
		return;
	}
	HeadIndex = InHeadIndex;
	HallPlayerName = InHallPlayerName;
	TeamType = InTeamType;
	JobType = InJobType;
	OnRep_PlayerInfoChanged();
}

void AHallPlayerState::SendChatMessageToSelf(EChatChannel Channel, const FText& Text)
{
	Client_SendChatMessageToSelf(Channel,Text);
	
}

void AHallPlayerState::SetReady(bool bNewReady)
{
	if (!HasAuthority())
	{
		Server_SetReady(bNewReady);
		return;
	}
	bReady = bNewReady;
	OnRep_bReady();
}

void AHallPlayerState::BeginPlay()
{
	Super::BeginPlay();
	//是否是房主
	if (HasAuthority())
	{
		bMaster = GetOwningController() == GetWorld()->GetFirstPlayerController();
		OnRep_bMaster();
	}
	
}

void AHallPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHallPlayerState,HeadIndex);
	DOREPLIFETIME(AHallPlayerState,HallPlayerName);
	DOREPLIFETIME(AHallPlayerState,TeamType);
	DOREPLIFETIME(AHallPlayerState,JobType);
	DOREPLIFETIME(AHallPlayerState,bReady);
	DOREPLIFETIME_CONDITION(AHallPlayerState,bMaster,COND_InitialOnly);
}

void AHallPlayerState::CopyProperties(APlayerState* PlayerState)
{
	if (ALgPlayerState* NewPs = Cast<ALgPlayerState>(PlayerState))
	{
		if (TeamType == ETeamType::ETT_None)
		{
			TeamType = ETeamType::ETT_Police;
		}
		if (JobType == EJobType::EJT_None)
		{
			JobType = EJobType::EJT_00;
		}
		NewPs->SetPlayerInfo(HallPlayerName,TeamType,JobType);
	}
	
	Super::CopyProperties(PlayerState);
}

void AHallPlayerState::OnRep_PlayerInfoChanged()
{
	if (OnPlayerInfoChanged.IsBound())
	{
		OnPlayerInfoChanged.Broadcast(HeadIndex,HallPlayerName,TeamType,JobType);
	}
}

void AHallPlayerState::OnRep_bReady()
{
	if (OnReadyChanged.IsBound())
	{
		OnReadyChanged.Broadcast(bReady);
	}
	
}

void AHallPlayerState::OnRep_bMaster()
{
	if (bMaster && OnIsMaster.IsBound())
	{
		OnIsMaster.Broadcast();
	}
	
}

void AHallPlayerState::Server_SetReady_Implementation(bool bNewReady)
{
	SetReady(bNewReady);
}

bool AHallPlayerState::Server_SetReady_Validate(bool bNewReady)
{
	return true;
}

void AHallPlayerState::Client_SendChatMessageToSelf_Implementation(EChatChannel Channel, const FText& Text)
{
	//到对应用户终端
	if (AHallHUD* HallHUD = Cast<AHallHUD>(GetWorld()->GetFirstPlayerController()->GetHUD()))
	{
		HallHUD->GetHallUserWidget()->PushMessage(Channel,Text);
	}
}

void AHallPlayerState::Server_SetPlayerInfo_Implementation(int32 InHeadIndex, const FString& InHallPlayerName,
                                                           ETeamType InTeamType, EJobType InJobType)
{
	SetPlayerInfo(InHeadIndex,InHallPlayerName,InTeamType,InJobType);
	
}

bool AHallPlayerState::Server_SetPlayerInfo_Validate(int32 InHeadIndex, const FString& InHallPlayerName,
	ETeamType InTeamType, EJobType InJobType)
{
	return true;
}
