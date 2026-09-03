#include "SurvivalGameState.h"

#include "Net/UnrealNetwork.h"

FTeamSurvivalState ASurvivalGameState::GetTeamState(ETeamType TeamType) const
{
	if (TeamType == ETeamType::ETT_Police)
	{
		return PoliceTeamState;
	}

	if (TeamType == ETeamType::ETT_Bandit)
	{
		return BanditTeamState;
	}

	FTeamSurvivalState EmptyState;
	EmptyState.TeamType = TeamType;
	return EmptyState;
}

void ASurvivalGameState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASurvivalGameState, MatchPhase);
	DOREPLIFETIME(ASurvivalGameState, SurvivalConfig);
	DOREPLIFETIME(ASurvivalGameState, bLayoutReady);
	DOREPLIFETIME(ASurvivalGameState, bWaitingForEligibleTeams);
	DOREPLIFETIME(ASurvivalGameState, PhaseStartServerTime);
	DOREPLIFETIME(ASurvivalGameState, SurvivalStartServerTime);
	DOREPLIFETIME(ASurvivalGameState, CountdownEndServerTime);
	DOREPLIFETIME(ASurvivalGameState, ActiveDifficultyPhaseIndex);
	DOREPLIFETIME(ASurvivalGameState, UnlockedRoomCount);
	DOREPLIFETIME(ASurvivalGameState, DirectorSnapshot);
	DOREPLIFETIME(ASurvivalGameState, PoliceTeamState);
	DOREPLIFETIME(ASurvivalGameState, BanditTeamState);
	DOREPLIFETIME(ASurvivalGameState, Outcome);
}

void ASurvivalGameState::SetSurvivalConfig(USurvivalModeConfig* NewConfig)
{
	if (SurvivalConfig == NewConfig)
	{
		return;
	}

	SurvivalConfig = NewConfig;
	OnRep_SurvivalMatchState();
}

void ASurvivalGameState::OnRep_SurvivalMatchState()
{
	OnSurvivalMatchStateChanged.Broadcast();
}

void ASurvivalGameState::OnRep_TeamState()
{
	OnSurvivalMatchStateChanged.Broadcast();
}

void ASurvivalGameState::SetMatchPhase(ESurvivalMatchPhase NewMatchPhase, float ServerTimeSeconds)
{
	if (MatchPhase == NewMatchPhase && PhaseStartServerTime == ServerTimeSeconds)
	{
		return;
	}

	MatchPhase = NewMatchPhase;
	PhaseStartServerTime = ServerTimeSeconds;
	OnRep_SurvivalMatchState();
}

void ASurvivalGameState::SetLayoutStatus(bool bNewLayoutReady, bool bNewWaitingForEligibleTeams)
{
	if (bLayoutReady == bNewLayoutReady && bWaitingForEligibleTeams == bNewWaitingForEligibleTeams)
	{
		return;
	}

	bLayoutReady = bNewLayoutReady;
	bWaitingForEligibleTeams = bNewWaitingForEligibleTeams;
	OnRep_SurvivalMatchState();
}

void ASurvivalGameState::SetSurvivalStartServerTime(float NewSurvivalStartServerTime)
{
	if (SurvivalStartServerTime == NewSurvivalStartServerTime)
	{
		return;
	}

	SurvivalStartServerTime = NewSurvivalStartServerTime;
	OnRep_SurvivalMatchState();
}

void ASurvivalGameState::SetCountdownEndServerTime(float NewCountdownEndServerTime)
{
	if (CountdownEndServerTime == NewCountdownEndServerTime)
	{
		return;
	}

	CountdownEndServerTime = NewCountdownEndServerTime;
	OnRep_SurvivalMatchState();
}

void ASurvivalGameState::SetActiveDifficultyPhaseIndex(int32 NewActiveDifficultyPhaseIndex)
{
	if (ActiveDifficultyPhaseIndex == NewActiveDifficultyPhaseIndex)
	{
		return;
	}

	ActiveDifficultyPhaseIndex = NewActiveDifficultyPhaseIndex;
	OnRep_SurvivalMatchState();
}

void ASurvivalGameState::SetUnlockedRoomCount(int32 NewUnlockedRoomCount)
{
	if (UnlockedRoomCount == NewUnlockedRoomCount)
	{
		return;
	}

	UnlockedRoomCount = NewUnlockedRoomCount;
	OnRep_SurvivalMatchState();
}

void ASurvivalGameState::SetDirectorSnapshot(const FSurvivalDirectorSnapshot& NewDirectorSnapshot)
{
	if (DirectorSnapshot.ResourceBudgetRemaining == NewDirectorSnapshot.ResourceBudgetRemaining
		&& DirectorSnapshot.EnemyBudgetRemaining == NewDirectorSnapshot.EnemyBudgetRemaining
		&& DirectorSnapshot.AliveEnemies == NewDirectorSnapshot.AliveEnemies)
	{
		return;
	}

	DirectorSnapshot = NewDirectorSnapshot;
	OnRep_SurvivalMatchState();
}

void ASurvivalGameState::SetTeamState(ETeamType TeamType, const FTeamSurvivalState& NewTeamState)
{
	FTeamSurvivalState* TargetState = nullptr;
	if (TeamType == ETeamType::ETT_Police)
	{
		TargetState = &PoliceTeamState;
	}
	else if (TeamType == ETeamType::ETT_Bandit)
	{
		TargetState = &BanditTeamState;
	}

	if (!TargetState)
	{
		return;
	}

	*TargetState = NewTeamState;
	OnRep_TeamState();
}

void ASurvivalGameState::SetOutcome(ESurvivalMatchOutcome NewOutcome)
{
	if (Outcome == NewOutcome)
	{
		return;
	}

	Outcome = NewOutcome;
	OnRep_SurvivalMatchState();
}
