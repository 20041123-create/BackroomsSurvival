#include "SurvivalPlayerState.h"

#include "Net/UnrealNetwork.h"

void ASurvivalPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASurvivalPlayerState, LifeState);
	DOREPLIFETIME(ASurvivalPlayerState, bMatchParticipant);
	DOREPLIFETIME(ASurvivalPlayerState, RespawnQueuePosition);
	DOREPLIFETIME(ASurvivalPlayerState, RespawnReadyServerTime);
	DOREPLIFETIME(ASurvivalPlayerState, DeathCount);
	DOREPLIFETIME(ASurvivalPlayerState, CompletedSurvivalSeconds);
}

void ASurvivalPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	if (ASurvivalPlayerState* SurvivalPlayerState = Cast<ASurvivalPlayerState>(PlayerState))
	{
		SurvivalPlayerState->LifeState = LifeState;
		SurvivalPlayerState->bMatchParticipant = bMatchParticipant;
		SurvivalPlayerState->RespawnQueuePosition = RespawnQueuePosition;
		SurvivalPlayerState->RespawnReadyServerTime = RespawnReadyServerTime;
		SurvivalPlayerState->DeathCount = DeathCount;
		SurvivalPlayerState->CompletedSurvivalSeconds = CompletedSurvivalSeconds;
	}
}

void ASurvivalPlayerState::OnRep_LifeState(ESurvivalLifeState PreviousLifeState)
{
	OnSurvivalLifeStateChanged.Broadcast();
}

void ASurvivalPlayerState::OnRep_SurvivalPlayerData()
{
	OnSurvivalLifeStateChanged.Broadcast();
}

void ASurvivalPlayerState::AssignServerMatchTeam(ETeamType NewTeamType)
{
	if (!HasAuthority() || (NewTeamType != ETeamType::ETT_Police && NewTeamType != ETeamType::ETT_Bandit))
	{
		return;
	}

	// This is deliberately not an RPC: the authoritative GameMode is the only caller.
	SetPlayerInfo(GetPlayerName(), NewTeamType, GetJobType());
}

void ASurvivalPlayerState::SetMatchParticipant(bool bNewMatchParticipant)
{
	if (bMatchParticipant == bNewMatchParticipant)
	{
		return;
	}

	bMatchParticipant = bNewMatchParticipant;
	OnRep_SurvivalPlayerData();
}

void ASurvivalPlayerState::SetLifeState(ESurvivalLifeState NewLifeState, float ServerTimeSeconds)
{
	if (LifeState == NewLifeState)
	{
		return;
	}

	if (LifeState == ESurvivalLifeState::Alive && AliveStateStartedServerTime > 0.0f)
	{
		CompletedSurvivalSeconds += FMath::Max(0.0f, ServerTimeSeconds - AliveStateStartedServerTime);
	}

	const ESurvivalLifeState PreviousLifeState = LifeState;
	LifeState = NewLifeState;
	AliveStateStartedServerTime = NewLifeState == ESurvivalLifeState::Alive ? ServerTimeSeconds : 0.0f;
	OnRep_LifeState(PreviousLifeState);
	OnRep_SurvivalPlayerData();
}

void ASurvivalPlayerState::SetRespawnState(int32 NewQueuePosition, float NewReadyServerTime)
{
	RespawnQueuePosition = NewQueuePosition;
	RespawnReadyServerTime = NewReadyServerTime;
	OnRep_SurvivalPlayerData();
}

void ASurvivalPlayerState::ClearRespawnState()
{
	SetRespawnState(INDEX_NONE, 0.0f);
}

void ASurvivalPlayerState::IncrementDeathCount()
{
	++DeathCount;
	OnRep_SurvivalPlayerData();
}
