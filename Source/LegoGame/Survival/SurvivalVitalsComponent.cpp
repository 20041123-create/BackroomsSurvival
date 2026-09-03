#include "SurvivalVitalsComponent.h"

#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "LegoGame/Character/LgCharacterBase.h"
#include "LegoGame/Components/PackageComponent.h"
#include "LegoGame/Survival/Contracts/SurvivalDataAssets.h"
#include "LegoGame/Survival/Contracts/SurvivalInterfaces.h"
#include "Net/UnrealNetwork.h"

USurvivalVitalsComponent::USurvivalVitalsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void USurvivalVitalsComponent::BeginPlay()
{
	Super::BeginPlay();
	bVitalsInitialized = true;

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Vitals.MaxHealth = DefaultMaxHealth * InitialDifficultyMultiplier;
		Vitals.MaxHunger = DefaultMaxHunger;
		Vitals.MaxThirst = DefaultMaxThirst;
		Vitals.Health = Vitals.MaxHealth;
		Vitals.Hunger = DefaultMaxHunger;
		Vitals.Thirst = DefaultMaxThirst;
		Vitals.LifeState = ESurvivalLifeState::Alive;
		NotifyVitalsChanged();
	}
}

bool USurvivalVitalsComponent::SetInitialDifficultyMultiplier(float DifficultyMultiplier)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || bVitalsInitialized || bInitialDifficultyConfigured
		|| !FMath::IsFinite(DifficultyMultiplier) || DifficultyMultiplier <= 0.0f)
	{
		return false;
	}

	InitialDifficultyMultiplier = DifficultyMultiplier;
	bInitialDifficultyConfigured = true;
	return true;
}

void USurvivalVitalsComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner() || !GetOwner()->HasAuthority() || IsDead())
	{
		return;
	}

	NeedsAccumulator += DeltaTime;
	while (NeedsAccumulator >= 1.0f)
	{
		NeedsAccumulator -= 1.0f;
		ApplyNeedsTick();
		if (IsDead())
		{
			break;
		}
	}
}

void USurvivalVitalsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USurvivalVitalsComponent, Vitals);
}

float USurvivalVitalsComponent::ApplyDamage(float DamageAmount, AController* InstigatorController, AActor* DamageCauser)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || IsDead() || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float AppliedDamage = FMath::Min(DamageAmount, Vitals.Health);
	Vitals.Health = FMath::Max(0.0f, Vitals.Health - DamageAmount);
	NotifyVitalsChanged();

	if (Vitals.Health <= 0.0f)
	{
		HandleDeath(InstigatorController, DamageCauser);
	}

	return AppliedDamage;
}

bool USurvivalVitalsComponent::ApplyConsumable(float HealthDelta, float HungerDelta, float ThirstDelta, int32 Quantity)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || IsDead() || Quantity <= 0)
	{
		return false;
	}

	const float NewHealth = FMath::Clamp(Vitals.Health + HealthDelta * Quantity, 0.0f, Vitals.MaxHealth);
	const float NewHunger = FMath::Clamp(Vitals.Hunger + HungerDelta * Quantity, 0.0f, Vitals.MaxHunger);
	const float NewThirst = FMath::Clamp(Vitals.Thirst + ThirstDelta * Quantity, 0.0f, Vitals.MaxThirst);
	if (FMath::IsNearlyEqual(NewHealth, Vitals.Health)
		&& FMath::IsNearlyEqual(NewHunger, Vitals.Hunger)
		&& FMath::IsNearlyEqual(NewThirst, Vitals.Thirst))
	{
		return false;
	}

	Vitals.Health = NewHealth;
	Vitals.Hunger = NewHunger;
	Vitals.Thirst = NewThirst;
	NotifyVitalsChanged();
	return true;
}

void USurvivalVitalsComponent::OnRep_Vitals()
{
	NotifyVitalsChanged();
}

void USurvivalVitalsComponent::ApplyNeedsTick()
{
	float HungerMultiplier = 1.0f;
	float ThirstMultiplier = 1.0f;
	GetDrainMultipliers(HungerMultiplier, ThirstMultiplier);

	Vitals.Hunger = FMath::Max(0.0f, Vitals.Hunger - HungerDrainPerSecond * HungerMultiplier);
	Vitals.Thirst = FMath::Max(0.0f, Vitals.Thirst - ThirstDrainPerSecond * ThirstMultiplier);
	if (Vitals.Hunger <= 0.0f || Vitals.Thirst <= 0.0f)
	{
		Vitals.Health = FMath::Max(0.0f, Vitals.Health - StarvationDamagePerSecond);
	}
	NotifyVitalsChanged();

	if (Vitals.Health <= 0.0f)
	{
		HandleDeath(nullptr, nullptr);
	}
}

void USurvivalVitalsComponent::NotifyVitalsChanged()
{
	OnVitalsChanged.Broadcast(Vitals);
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		GetOwner()->ForceNetUpdate();
	}
}

void USurvivalVitalsComponent::HandleDeath(AController* InstigatorController, AActor* DamageCauser)
{
	if (IsDead())
	{
		return;
	}

	Vitals.LifeState = ESurvivalLifeState::WaitingRespawn;
	NotifyVitalsChanged();

	if (ALgCharacterBase* Character = Cast<ALgCharacterBase>(GetOwner()))
	{
		Character->StopFire();
		if (UPackageComponent* Package = Character->GetPackageComponent())
		{
			Package->DropAllSurvivalItems();
		}
	}

	if (UWorld* World = GetWorld())
	{
		// GameMode owns authoritative death, spectator and respawn queues. Keep a
		// GameState fallback for modes that intentionally expose a listener there.
		UObject* DeathListener = World->GetAuthGameMode();
		if (!DeathListener || !DeathListener->GetClass()->ImplementsInterface(USurvivalDeathListenerInterface::StaticClass()))
		{
			DeathListener = World->GetGameState();
		}
		if (DeathListener && DeathListener->GetClass()->ImplementsInterface(USurvivalDeathListenerInterface::StaticClass()))
		{
			ISurvivalDeathListenerInterface::Execute_HandleSurvivalDeath(
				DeathListener, GetOwner(), InstigatorController, DamageCauser);
		}
		else
		{
			UE_LOG(LogTemp, Error,
				TEXT("Survival death for %s has no authoritative death-listener provider."),
				*GetNameSafe(GetOwner()));
		}

		if (const APawn* DeadPawn = Cast<APawn>(GetOwner()))
		{
			if (Cast<APlayerController>(DeadPawn->GetController()))
			{
				UE_LOG(LogTemp, Error,
					TEXT("Survival death listener left dead player Pawn %s possessed; check the preceding Match rejection reason."),
					*GetNameSafe(DeadPawn));
			}
		}
	}
}

void USurvivalVitalsComponent::GetDrainMultipliers(float& OutHungerMultiplier, float& OutThirstMultiplier) const
{
	OutHungerMultiplier = 1.0f;
	OutThirstMultiplier = 1.0f;

	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!GameState || !GameState->GetClass()->ImplementsInterface(USurvivalMatchStateInterface::StaticClass()))
	{
		return;
	}

	const USurvivalModeConfig* Config = ISurvivalMatchStateInterface::Execute_GetSurvivalConfig(const_cast<AGameStateBase*>(GameState));
	if (!Config || Config->Phases.IsEmpty())
	{
		return;
	}

	const float ElapsedSeconds = World->GetTimeSeconds();
	const FSurvivalPhaseDefinition* ActivePhase = &Config->Phases[0];
	for (const FSurvivalPhaseDefinition& Phase : Config->Phases)
	{
		if (Phase.StartTimeSeconds <= ElapsedSeconds && Phase.StartTimeSeconds >= ActivePhase->StartTimeSeconds)
		{
			ActivePhase = &Phase;
		}
	}

	OutHungerMultiplier = ActivePhase->HungerDrainMultiplier;
	OutThirstMultiplier = ActivePhase->ThirstDrainMultiplier;
}
