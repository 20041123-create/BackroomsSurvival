// Fill out your copyright notice in the Description page of Project Settings.


#include "LgPlayerState.h"

#include "LegoGame/Character/LgCharacterBase.h"
#include "Net/UnrealNetwork.h"



void ALgPlayerState::SetPlayerInfo(const FString& InPlayerName, ETeamType InTeamType, EJobType InJobType)
{
	PlayerName = InPlayerName;
	TeamType = InTeamType;
	JobType = InJobType;
	
}

void ALgPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALgPlayerState,PlayerName);
	DOREPLIFETIME(ALgPlayerState,TeamType);
	DOREPLIFETIME(ALgPlayerState,JobType);
	
}

void ALgPlayerState::OnRep_PlayerJobInfo()
{
	if (APawn* MyPawn = GetPawn())
	{
		if (ALgCharacterBase* MyChar = Cast<ALgCharacterBase>(MyPawn))
		{
			MyChar->UpdateJobMesh();
		}
	}
	
}
