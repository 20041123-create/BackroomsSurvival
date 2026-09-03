#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LegoGame/Survival/Contracts/SurvivalInterfaces.h"
#include "LegoGame/Survival/Contracts/SurvivalTypes.h"
#include "SurvivalGameMode.generated.h"

class ASurvivalGameState;
class ASurvivalPlayerState;
class USurvivalMatchIntegrationStub;
class USurvivalModeConfig;
struct FSurvivalMatchProductionTestAccess;
struct FSurvivalMapConfigurationTestAccess;

/** Server-authoritative rules, phase state machine, directors, respawns, and victory logic. */
UCLASS()
class LEGOGAME_API ASurvivalGameMode : public AGameMode, public ISurvivalDeathListenerInterface
{
	GENERATED_BODY()

public:
	ASurvivalGameMode();

	/** Trusted server-only hook for the future team-terminal integration and development tests. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Survival|Development", meta=(DeprecatedFunction, DeprecationMessage="Shipping gameplay must deposit Item.RespawnEnergy through DepositTeamRespawnEnergy."))
	void AddTeamRespawnEnergy(ETeamType TeamType, int32 Amount);

	/** Server-authoritative deposit which atomically consumes Item.RespawnEnergy from one inventory owner. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Survival|Respawn")
	bool DepositTeamRespawnEnergy(AActor* SourceInventoryOwner, ETeamType TeamType, int32 Quantity);

	/** Development-only stub hook; production enemy tracking uses HandleSurvivalDeath. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Survival|Development", meta=(DeprecatedFunction, DeprecationMessage="Production enemy lifecycle is driven by HandleSurvivalDeath."))
	void NotifyDirectorEnemyDefeated();

	virtual void HandleSurvivalDeath_Implementation(
		AActor* Victim,
		AController* InstigatorController,
		AActor* DamageCauser) override;

protected:
	virtual void StartPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	UPROPERTY(EditDefaultsOnly, Category="Survival|Configuration")
	TSoftObjectPtr<USurvivalModeConfig> ModeConfig;

	UPROPERTY(EditDefaultsOnly, Category="Survival|Timing", meta=(ClampMin="0.0"))
	float CountdownDurationSeconds = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category="Survival|Timing", meta=(ClampMin="1.0"))
	float FinalPhaseDirectorBudgetWindowSeconds = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category="Survival|Timing", meta=(ClampMin="0.05"))
	float MatchUpdateIntervalSeconds = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category="Survival|Development")
	bool bUseDevelopmentIntegrationStubs = false;

	UPROPERTY(EditDefaultsOnly, Category="Survival|Development")
	bool bAllowDevelopmentFallbackConfig = true;

	/** Formal resource tags selected deterministically by the authoritative Match random stream. */
	UPROPERTY(EditDefaultsOnly, Category="Survival|Directors")
	TArray<FGameplayTag> ResourceItemTags;

	UPROPERTY(EditDefaultsOnly, Category="Survival|Directors", meta=(ClampMin="1"))
	int32 ResourceSpawnQuantity = 1;

	/** Ammo pickups contain rounds; all other resource pickups retain ResourceSpawnQuantity. */
	UPROPERTY(EditDefaultsOnly, Category="Survival|Directors", meta=(ClampMin="1"))
	int32 AmmoResourceSpawnQuantity = 30;

	/** Invalid uses the Core-configured default enemy archetype. */
	UPROPERTY(EditDefaultsOnly, Category="Survival|Directors")
	FGameplayTag EnemyArchetypeTag;

private:
	friend struct FSurvivalMatchProductionTestAccess;
	friend struct FSurvivalMapConfigurationTestAccess;

	struct FRespawnQueueEntry
	{
		TWeakObjectPtr<APlayerController> Controller;
		TWeakObjectPtr<ASurvivalPlayerState> PlayerState;
		bool bEnergyReserved = false;
		float RespawnReadyServerTime = 0.0f;
	};

	struct FTeamRuntimeState
	{
		ETeamType TeamType = ETeamType::ETT_None;
		int32 RespawnEnergy = 0;
		bool bEliminated = false;
		TArray<FRespawnQueueEntry> RespawnQueue;
	};

	struct FDirectorRuntimeState
	{
		int32 RemainingBudget = 0;
		float NextAttemptServerTime = 0.0f;
		float AttemptIntervalSeconds = 0.0f;
	};

	void InitializeMatch();
	bool ResolveAndValidateConfig();
	USurvivalModeConfig* BuildDevelopmentFallbackConfig();
	bool ValidateConfig(TArray<FSurvivalPhaseDefinition>& OutSortedPhases, FString& OutFailureReason) const;

	void OnStubLayoutReady(bool bSucceeded);
	bool DiscoverRuntimeProviders();
	bool HasValidRuntimeProviders() const;
	bool UpdateProductionLayout();
	bool GetProductionLayoutSnapshot(FSurvivalWorldRuntimeSnapshot& OutSnapshot) const;
	bool AdvanceWorldToPhase(const FSurvivalPhaseDefinition& Phase, FSurvivalWorldRuntimeSnapshot& OutSnapshot);
	bool SelectEnabledAnchor(FGameplayTag AnchorTag, FSurvivalAnchorView& OutAnchor);
	int32 GetResourceSpawnQuantityForTag(FGameplayTag ItemTag) const;
	bool TrySpawnDirectedResource();
	bool TrySpawnDirectedEnemy(const FSurvivalPhaseDefinition& Phase);
	void PruneAliveEnemyActors();
	void UpdateMatch();
	void TryStartCountdown();
	void ReturnToWaitingForTeams();
	void StartSurvival();
	void UpdateDifficultyPhase(float SurvivalElapsedSeconds);
	void ApplyDifficultyPhase(int32 SortedPhaseArrayIndex);
	void AdvanceDirectors(float ServerTimeSeconds);
	void RefreshDirectorSnapshot();

	void RegisterController(APlayerController* Controller);
	void UnregisterController(APlayerController* Controller);
	void PruneInvalidControllers();
	/** Removes a pre-travel/default pawn so RestartPlayerAtTransform must create one at the runtime anchor. */
	void DestroyExistingPlayerPawn(APlayerController* Controller);
	void EnterTemporarySpectatorState(APlayerController* Controller);
	void AssignFallbackTeamIfNeeded(ASurvivalPlayerState& PlayerState);
	ETeamType DetermineBalancedFallbackTeam(int32 PoliceCount, int32 BanditCount);
	bool HasEligibleTeam(ETeamType TeamType) const;
	bool IsValidPlayerTeam(ETeamType TeamType) const;

	void EnqueueRespawn(APlayerController* Controller, ASurvivalPlayerState* PlayerState);
	void RemoveFromRespawnQueue(ASurvivalPlayerState* PlayerState, bool bRefundReservedEnergy);
	void ReserveRespawnEnergy(FTeamRuntimeState& TeamState, float ServerTimeSeconds);
	void ProcessRespawnQueues(float ServerTimeSeconds);
	void RefreshRespawnQueueState(FTeamRuntimeState& TeamState);
	bool RestartPlayerForTeam(APlayerController* Controller, ETeamType TeamType);

	void RefreshReplicatedTeamStates();
	void ScheduleOutcomeEvaluation();
	void EvaluateOutcome();
	bool IsTeamViable(const FTeamRuntimeState& TeamState) const;
	void CompleteMatch(ETeamType WinningTeam, bool bDraw);

	ASurvivalGameState* GetSurvivalGameState() const;
	FTeamRuntimeState* GetTeamRuntimeState(ETeamType TeamType);
	const FTeamRuntimeState* GetTeamRuntimeState(ETeamType TeamType) const;
	float GetServerTimeSeconds() const;
	float GetCurrentRespawnDelaySeconds() const;
	int32 GetCurrentRespawnEnergyCost() const;
	float GetCurrentPhaseBudgetWindowSeconds(int32 SortedPhaseArrayIndex) const;

	UPROPERTY(Transient)
	TObjectPtr<USurvivalModeConfig> ResolvedConfig;

	UPROPERTY(Transient)
	TObjectPtr<USurvivalMatchIntegrationStub> IntegrationStub;

	TArray<FSurvivalPhaseDefinition> RuntimePhases;
	TSet<TWeakObjectPtr<APlayerController>> RegisteredControllers;
	TSet<TWeakObjectPtr<AActor>> AliveEnemyActors;
	FTeamRuntimeState PoliceTeamState;
	FTeamRuntimeState BanditTeamState;
	FDirectorRuntimeState ResourceDirector;
	FDirectorRuntimeState EnemyDirector;
	FTimerHandle MatchUpdateTimer;
	FRandomStream MatchRandomStream;
	int32 ActiveSortedPhaseArrayIndex = INDEX_NONE;
	TWeakObjectPtr<AActor> WorldRuntimeProvider;
	TWeakObjectPtr<AActor> RuntimeSpawnProvider;
	bool bConfigurationValid = false;
	bool bLayoutRequestIssued = false;
	bool bRuntimeProvidersValid = false;
	bool bLayoutFailureLogged = false;
	bool bOutcomeEvaluationPending = false;
	/** Stable alternating tie-breaker for direct joins with ETT_None. */
	bool bNextBalancedTeamIsPolice = true;
};
