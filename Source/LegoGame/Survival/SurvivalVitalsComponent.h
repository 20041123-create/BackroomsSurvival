#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LegoGame/Survival/Contracts/SurvivalTypes.h"
#include "SurvivalVitalsComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FSurvivalVitalsChanged, const FSurvivalVitalsSnapshot&);

UCLASS(ClassGroup=(Survival), meta=(BlueprintSpawnableComponent))
class LEGOGAME_API USurvivalVitalsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivalVitalsComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="Survival|Vitals")
	FSurvivalVitalsSnapshot GetSnapshot() const { return Vitals; }

	float ApplyDamage(float DamageAmount, AController* InstigatorController, AActor* DamageCauser);
	bool ApplyConsumable(float HealthDelta, float HungerDelta, float ThirstDelta, int32 Quantity);
	bool IsDead() const { return Vitals.LifeState != ESurvivalLifeState::Alive; }
	/** Server-only initial-spawn configuration. It can be accepted exactly once before BeginPlay. */
	bool SetInitialDifficultyMultiplier(float DifficultyMultiplier);

	FSurvivalVitalsChanged OnVitalsChanged;

protected:
	UFUNCTION()
	void OnRep_Vitals();

	void ApplyNeedsTick();
	void NotifyVitalsChanged();
	void HandleDeath(AController* InstigatorController, AActor* DamageCauser);
	void GetDrainMultipliers(float& OutHungerMultiplier, float& OutThirstMultiplier) const;

	UPROPERTY(EditDefaultsOnly, Category="Survival|Vitals", meta=(ClampMin="0.0"))
	float DefaultMaxHealth = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category="Survival|Vitals", meta=(ClampMin="0.0"))
	float DefaultMaxHunger = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category="Survival|Vitals", meta=(ClampMin="0.0"))
	float DefaultMaxThirst = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category="Survival|Vitals", meta=(ClampMin="0.0"))
	float HungerDrainPerSecond = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category="Survival|Vitals", meta=(ClampMin="0.0"))
	float ThirstDrainPerSecond = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category="Survival|Vitals", meta=(ClampMin="0.0"))
	float StarvationDamagePerSecond = 1.0f;

	UPROPERTY(ReplicatedUsing=OnRep_Vitals, VisibleInstanceOnly, Category="Survival|Vitals")
	FSurvivalVitalsSnapshot Vitals;

	float InitialDifficultyMultiplier = 1.0f;
	bool bInitialDifficultyConfigured = false;
	bool bVitalsInitialized = false;
	float NeedsAccumulator = 0.0f;
};
