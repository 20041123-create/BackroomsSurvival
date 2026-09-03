#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "LegoGame/Survival/Contracts/SurvivalInterfaces.h"
#include "LegoGame/Survival/Contracts/SurvivalTypes.h"
#include "LegoGame/Survival/Match/SurvivalMatchTypes.h"
#include "SurvivalGameState.generated.h"

DECLARE_MULTICAST_DELEGATE(FSurvivalMatchStateChanged);

class USurvivalModeConfig;
struct FSurvivalMatchProductionTestAccess;

/** Replicated, client-readable snapshot of the authoritative Survival match. */
UCLASS()
class LEGOGAME_API ASurvivalGameState : public AGameState, public ISurvivalMatchStateInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category="Survival|Match")
	ESurvivalMatchPhase GetSurvivalMatchPhase() const;

	virtual USurvivalModeConfig* GetSurvivalConfig_Implementation() const override { return SurvivalConfig; }
	virtual ESurvivalMatchPhase GetSurvivalMatchPhase_Implementation() const override { return MatchPhase; }

	UFUNCTION(BlueprintPure, Category="Survival|Match")
	bool IsLayoutReady() const { return bLayoutReady; }

	UFUNCTION(BlueprintPure, Category="Survival|Match")
	bool IsWaitingForEligibleTeams() const { return bWaitingForEligibleTeams; }

	UFUNCTION(BlueprintPure, Category="Survival|Match")
	float GetPhaseStartServerTime() const { return PhaseStartServerTime; }

	UFUNCTION(BlueprintPure, Category="Survival|Match")
	float GetSurvivalStartServerTime() const { return SurvivalStartServerTime; }

	UFUNCTION(BlueprintPure, Category="Survival|Match")
	float GetCountdownEndServerTime() const { return CountdownEndServerTime; }

	UFUNCTION(BlueprintPure, Category="Survival|Match")
	int32 GetActiveDifficultyPhaseIndex() const { return ActiveDifficultyPhaseIndex; }

	UFUNCTION(BlueprintPure, Category="Survival|Match")
	int32 GetUnlockedRoomCount() const { return UnlockedRoomCount; }

	UFUNCTION(BlueprintPure, Category="Survival|Match")
	FSurvivalDirectorSnapshot GetDirectorSnapshot() const { return DirectorSnapshot; }

	UFUNCTION(BlueprintPure, Category="Survival|Match")
	FTeamSurvivalState GetTeamState(ETeamType TeamType) const;

	UFUNCTION(BlueprintPure, Category="Survival|Match")
	ESurvivalMatchOutcome GetOutcome() const { return Outcome; }

	FSurvivalMatchStateChanged OnSurvivalMatchStateChanged;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_SurvivalMatchState();

	UFUNCTION()
	void OnRep_TeamState();

private:
	friend class ASurvivalGameMode;
	friend struct FSurvivalMatchProductionTestAccess;

	void SetSurvivalConfig(USurvivalModeConfig* NewConfig);
	void SetMatchPhase(ESurvivalMatchPhase NewMatchPhase, float ServerTimeSeconds);
	void SetLayoutStatus(bool bNewLayoutReady, bool bNewWaitingForEligibleTeams);
	void SetSurvivalStartServerTime(float NewSurvivalStartServerTime);
	void SetCountdownEndServerTime(float NewCountdownEndServerTime);
	void SetActiveDifficultyPhaseIndex(int32 NewActiveDifficultyPhaseIndex);
	void SetUnlockedRoomCount(int32 NewUnlockedRoomCount);
	void SetDirectorSnapshot(const FSurvivalDirectorSnapshot& NewDirectorSnapshot);
	void SetTeamState(ETeamType TeamType, const FTeamSurvivalState& NewTeamState);
	void SetOutcome(ESurvivalMatchOutcome NewOutcome);

	UPROPERTY(ReplicatedUsing=OnRep_SurvivalMatchState)
	ESurvivalMatchPhase MatchPhase = ESurvivalMatchPhase::WaitingForLayout;

	UPROPERTY(ReplicatedUsing=OnRep_SurvivalMatchState)
	TObjectPtr<USurvivalModeConfig> SurvivalConfig;

	UPROPERTY(ReplicatedUsing=OnRep_SurvivalMatchState)
	bool bLayoutReady = false;

	UPROPERTY(ReplicatedUsing=OnRep_SurvivalMatchState)
	bool bWaitingForEligibleTeams = true;

	UPROPERTY(ReplicatedUsing=OnRep_SurvivalMatchState)
	float PhaseStartServerTime = 0.0f;

	UPROPERTY(ReplicatedUsing=OnRep_SurvivalMatchState)
	float SurvivalStartServerTime = 0.0f;

	UPROPERTY(ReplicatedUsing=OnRep_SurvivalMatchState)
	float CountdownEndServerTime = 0.0f;

	UPROPERTY(ReplicatedUsing=OnRep_SurvivalMatchState)
	int32 ActiveDifficultyPhaseIndex = INDEX_NONE;

	UPROPERTY(ReplicatedUsing=OnRep_SurvivalMatchState)
	int32 UnlockedRoomCount = 0;

	UPROPERTY(ReplicatedUsing=OnRep_SurvivalMatchState)
	FSurvivalDirectorSnapshot DirectorSnapshot;

	UPROPERTY(ReplicatedUsing=OnRep_TeamState)
	FTeamSurvivalState PoliceTeamState;

	UPROPERTY(ReplicatedUsing=OnRep_TeamState)
	FTeamSurvivalState BanditTeamState;

	UPROPERTY(ReplicatedUsing=OnRep_SurvivalMatchState)
	ESurvivalMatchOutcome Outcome = ESurvivalMatchOutcome::None;
};
