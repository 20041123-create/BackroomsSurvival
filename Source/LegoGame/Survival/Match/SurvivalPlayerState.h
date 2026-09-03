#pragma once

#include "CoreMinimal.h"
#include "LegoGame/GamePlay/MainGame/LgPlayerState.h"
#include "LegoGame/Survival/Contracts/SurvivalTypes.h"
#include "SurvivalPlayerState.generated.h"

DECLARE_MULTICAST_DELEGATE(FSurvivalPlayerLifeStateChanged);

struct FSurvivalMatchProductionTestAccess;

/** Per-player replicated Survival state. All mutations are owned by ASurvivalGameMode. */
UCLASS()
class LEGOGAME_API ASurvivalPlayerState : public ALgPlayerState
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Survival|Player")
	ESurvivalLifeState GetSurvivalLifeState() const { return LifeState; }

	UFUNCTION(BlueprintPure, Category="Survival|Player")
	bool IsMatchParticipant() const { return bMatchParticipant; }

	UFUNCTION(BlueprintPure, Category="Survival|Player")
	int32 GetRespawnQueuePosition() const { return RespawnQueuePosition; }

	UFUNCTION(BlueprintPure, Category="Survival|Player")
	float GetRespawnReadyServerTime() const { return RespawnReadyServerTime; }

	UFUNCTION(BlueprintPure, Category="Survival|Player")
	int32 GetDeathCount() const { return DeathCount; }

	UFUNCTION(BlueprintPure, Category="Survival|Player")
	float GetCompletedSurvivalSeconds() const { return CompletedSurvivalSeconds; }

	FSurvivalPlayerLifeStateChanged OnSurvivalLifeStateChanged;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void CopyProperties(APlayerState* PlayerState) override;

	UFUNCTION()
	void OnRep_LifeState(ESurvivalLifeState PreviousLifeState);

	UFUNCTION()
	void OnRep_SurvivalPlayerData();

private:
	friend class ASurvivalGameMode;
	friend struct FSurvivalMatchProductionTestAccess;

	/** Server-only Match fallback for direct Survival map joins that do not carry a lobby team. */
	void AssignServerMatchTeam(ETeamType NewTeamType);
	void SetMatchParticipant(bool bNewMatchParticipant);
	void SetLifeState(ESurvivalLifeState NewLifeState, float ServerTimeSeconds);
	void SetRespawnState(int32 NewQueuePosition, float NewReadyServerTime);
	void ClearRespawnState();
	void IncrementDeathCount();

	UPROPERTY(ReplicatedUsing=OnRep_LifeState)
	ESurvivalLifeState LifeState = ESurvivalLifeState::Spectating;

	UPROPERTY(ReplicatedUsing=OnRep_SurvivalPlayerData)
	bool bMatchParticipant = false;

	UPROPERTY(ReplicatedUsing=OnRep_SurvivalPlayerData)
	int32 RespawnQueuePosition = INDEX_NONE;

	UPROPERTY(ReplicatedUsing=OnRep_SurvivalPlayerData)
	float RespawnReadyServerTime = 0.0f;

	UPROPERTY(ReplicatedUsing=OnRep_SurvivalPlayerData)
	int32 DeathCount = 0;

	UPROPERTY(ReplicatedUsing=OnRep_SurvivalPlayerData)
	float CompletedSurvivalSeconds = 0.0f;

	float AliveStateStartedServerTime = 0.0f;
};
