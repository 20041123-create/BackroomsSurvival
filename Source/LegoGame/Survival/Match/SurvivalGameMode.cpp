#include "SurvivalGameMode.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "LegoGame/Player/LgPlayerController.h"
#include "LegoGame/Player/PlayerCharacter.h"
#include "LegoGame/Survival/Contracts/SurvivalDataAssets.h"
#include "LegoGame/Survival/Contracts/SurvivalGameplayTags.h"
#include "LegoGame/Survival/Match/Integration/Stubs/SurvivalMatchIntegrationStub.h"
#include "LegoGame/Survival/Match/SurvivalGameState.h"
#include "LegoGame/Survival/Match/SurvivalHUD.h"
#include "LegoGame/Survival/Match/SurvivalMatchTypes.h"
#include "LegoGame/Survival/Match/SurvivalPlayerState.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalMatchRespawn, Log, All);

ASurvivalGameMode::ASurvivalGameMode()
{
	static ConstructorHelpers::FClassFinder<APlayerCharacter> PlayerClass(
		TEXT("/Script/Engine.Blueprint'/Game/LegoGame/Blueprints/Player/BP_TestPlayer.BP_TestPlayer_C'"));
	DefaultPawnClass = PlayerClass.Class;
	PlayerControllerClass = ALgPlayerController::StaticClass();
	PlayerStateClass = ASurvivalPlayerState::StaticClass();
	GameStateClass = ASurvivalGameState::StaticClass();
	HUDClass = ASurvivalHUD::StaticClass();
	bUseSeamlessTravel = true;

	PoliceTeamState.TeamType = ETeamType::ETT_Police;
	BanditTeamState.TeamType = ETeamType::ETT_Bandit;
	ResourceItemTags = {
		LG::SurvivalTags::Item_Food,
		LG::SurvivalTags::Item_Water,
		LG::SurvivalTags::Item_Material,
		LG::SurvivalTags::Item_Ammo
	};

#if UE_BUILD_SHIPPING
	bUseDevelopmentIntegrationStubs = false;
	bAllowDevelopmentFallbackConfig = false;
#endif
}

void ASurvivalGameMode::StartPlay()
{
	Super::StartPlay();
	InitializeMatch();
}

void ASurvivalGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	RegisterController(NewPlayer);
}

void ASurvivalGameMode::Logout(AController* Exiting)
{
	if (APlayerController* PlayerController = Cast<APlayerController>(Exiting))
	{
		if (ASurvivalPlayerState* PlayerState = PlayerController->GetPlayerState<ASurvivalPlayerState>())
		{
			RemoveFromRespawnQueue(PlayerState, true);
			if (PlayerState->IsMatchParticipant())
			{
				PlayerState->SetLifeState(ESurvivalLifeState::Eliminated, GetServerTimeSeconds());
			}
		}

		UnregisterController(PlayerController);
		RefreshReplicatedTeamStates();
		ScheduleOutcomeEvaluation();
	}

	Super::Logout(Exiting);
}

void ASurvivalGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// Survival owns initial spawning: players wait without becoming permanent spectators.
	EnterTemporarySpectatorState(NewPlayer);
}

void ASurvivalGameMode::AddTeamRespawnEnergy(ETeamType TeamType, int32 Amount)
{
	if (!HasAuthority() || !bUseDevelopmentIntegrationStubs || Amount <= 0)
	{
		return;
	}

	ASurvivalGameState* SurvivalGameState = GetSurvivalGameState();
	if (!SurvivalGameState || SurvivalGameState->GetSurvivalMatchPhase() != ESurvivalMatchPhase::InProgress)
	{
		return;
	}

	if (FTeamRuntimeState* TeamState = GetTeamRuntimeState(TeamType))
	{
		if (TeamState->bEliminated)
		{
			return;
		}

		TeamState->RespawnEnergy += Amount;
		ReserveRespawnEnergy(*TeamState, GetServerTimeSeconds());
		RefreshReplicatedTeamStates();
		ScheduleOutcomeEvaluation();
	}
}

bool ASurvivalGameMode::DepositTeamRespawnEnergy(AActor* SourceInventoryOwner, ETeamType TeamType, int32 Quantity)
{
	if (!HasAuthority() || !IsValid(SourceInventoryOwner) || !IsValidPlayerTeam(TeamType) || Quantity <= 0)
	{
		return false;
	}

	ASurvivalGameState* SurvivalGameState = GetSurvivalGameState();
	FTeamRuntimeState* TeamState = GetTeamRuntimeState(TeamType);
	if (!SurvivalGameState || SurvivalGameState->GetSurvivalMatchPhase() != ESurvivalMatchPhase::InProgress
		|| !TeamState || TeamState->bEliminated
		|| !SourceInventoryOwner->GetClass()->ImplementsInterface(USurvivalInventoryInterface::StaticClass()))
	{
		return false;
	}

	if (const APawn* SourcePawn = Cast<APawn>(SourceInventoryOwner))
	{
		const ASurvivalPlayerState* SourcePlayerState = SourcePawn->GetPlayerState<ASurvivalPlayerState>();
		if (SourcePlayerState && SourcePlayerState->GetTeamType() != TeamType)
		{
			return false;
		}
	}

	if (!ISurvivalInventoryInterface::Execute_TryConsumeItemsByTag(
		SourceInventoryOwner, LG::SurvivalTags::Item_RespawnEnergy, Quantity))
	{
		return false;
	}

	TeamState->RespawnEnergy += Quantity;
	ReserveRespawnEnergy(*TeamState, GetServerTimeSeconds());
	RefreshReplicatedTeamStates();
	ScheduleOutcomeEvaluation();
	return true;
}

void ASurvivalGameMode::NotifyDirectorEnemyDefeated()
{
	if (!HasAuthority() || !bUseDevelopmentIntegrationStubs || !IntegrationStub)
	{
		return;
	}

	IntegrationStub->NotifyStubEnemyDefeated();
	RefreshDirectorSnapshot();
}

void ASurvivalGameMode::HandleSurvivalDeath_Implementation(
	AActor* Victim,
	AController* InstigatorController,
	AActor* DamageCauser)
{
	if (!HasAuthority())
	{
		return;
	}

	ASurvivalGameState* SurvivalGameState = GetSurvivalGameState();
	if (!SurvivalGameState || SurvivalGameState->GetSurvivalMatchPhase() != ESurvivalMatchPhase::InProgress)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Survival death ignored for %s: authoritative Survival phase is %d."),
			*GetNameSafe(Victim), SurvivalGameState
				? static_cast<uint8>(SurvivalGameState->GetSurvivalMatchPhase())
				: INDEX_NONE);
		return;
	}

	APawn* VictimPawn = Cast<APawn>(Victim);
	APlayerController* VictimController = VictimPawn ? Cast<APlayerController>(VictimPawn->GetController()) : nullptr;
	ASurvivalPlayerState* VictimPlayerState = VictimPawn
		? VictimPawn->GetPlayerState<ASurvivalPlayerState>()
		: nullptr;
	if (!VictimPlayerState && VictimController)
	{
		VictimPlayerState = VictimController->GetPlayerState<ASurvivalPlayerState>();
	}
	if (!VictimController && VictimPlayerState)
	{
		VictimController = Cast<APlayerController>(VictimPlayerState->GetOwner());
	}

	if (!VictimController || !VictimPlayerState)
	{
		if (AliveEnemyActors.Remove(Victim) > 0)
		{
			RefreshDirectorSnapshot();
		}
		else if (VictimPawn)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("Survival death ignored for Pawn %s: Controller=%s PlayerState=%s."),
				*GetNameSafe(VictimPawn), *GetNameSafe(VictimController), *GetNameSafe(VictimPlayerState));
		}
		return;
	}

	if (!IsValidPlayerTeam(VictimPlayerState->GetTeamType()))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Survival player death ignored for %s: invalid team %d."),
			*GetNameSafe(Victim), static_cast<uint8>(VictimPlayerState->GetTeamType()));
		return;
	}
	if (VictimController->GetPawn() != VictimPawn)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Survival player death ignored for %s: controller %s possesses %s."),
			*GetNameSafe(Victim), *GetNameSafe(VictimController), *GetNameSafe(VictimController->GetPawn()));
		return;
	}

	const ESurvivalLifeState PreviousLifeState = VictimPlayerState->GetSurvivalLifeState();
	if (PreviousLifeState == ESurvivalLifeState::WaitingRespawn
		|| PreviousLifeState == ESurvivalLifeState::Respawning
		|| PreviousLifeState == ESurvivalLifeState::Eliminated)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("Duplicate Survival player death ignored for %s in life state %d."),
			*GetNameSafe(Victim), static_cast<uint8>(PreviousLifeState));
		return;
	}

	// Possession of the reported Pawn during an active Survival match is the
	// authoritative gameplay fact. Recover stale participant/life metadata from
	// travel or replication ordering instead of leaving a dead Pawn permanently
	// possessed after its vitals component has already dropped the inventory.
	if (!VictimPlayerState->IsMatchParticipant() || PreviousLifeState != ESurvivalLifeState::Alive)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Recovering Survival player death state for %s: participant=%d life=%d controller=%s."),
			*GetNameSafe(Victim), VictimPlayerState->IsMatchParticipant() ? 1 : 0,
			static_cast<uint8>(PreviousLifeState), *GetNameSafe(VictimController));
		VictimPlayerState->SetMatchParticipant(true);
	}
	RegisteredControllers.Add(VictimController);

	const float ServerTimeSeconds = GetServerTimeSeconds();
	VictimPlayerState->IncrementDeathCount();
	VictimPlayerState->SetLifeState(ESurvivalLifeState::WaitingRespawn, ServerTimeSeconds);
	EnterTemporarySpectatorState(VictimController);
	EnqueueRespawn(VictimController, VictimPlayerState);
	UE_LOG(LogSurvivalMatchRespawn, Display,
		TEXT("Survival death queued for %s: team=%d queue=%d ready=%.2f energy=%d."),
		*GetNameSafe(VictimPlayerState), static_cast<uint8>(VictimPlayerState->GetTeamType()),
		VictimPlayerState->GetRespawnQueuePosition(), VictimPlayerState->GetRespawnReadyServerTime(),
		GetTeamRuntimeState(VictimPlayerState->GetTeamType())
			? GetTeamRuntimeState(VictimPlayerState->GetTeamType())->RespawnEnergy
			: INDEX_NONE);
	RefreshReplicatedTeamStates();
	ScheduleOutcomeEvaluation();
}

void ASurvivalGameMode::InitializeMatch()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!ResolveAndValidateConfig())
	{
		UE_LOG(LogTemp, Error, TEXT("Survival Match configuration is invalid; remaining in WaitingForLayout."));
		return;
	}
	if (ASurvivalGameState* SurvivalGameState = GetSurvivalGameState())
	{
		SurvivalGameState->SetSurvivalConfig(ResolvedConfig);
	}

	MatchRandomStream.Initialize(ResolvedConfig->RandomSeed);
	bLayoutRequestIssued = false;
	bLayoutFailureLogged = false;

	if (bUseDevelopmentIntegrationStubs)
	{
		IntegrationStub = NewObject<USurvivalMatchIntegrationStub>(this);
		bLayoutRequestIssued = true;
		IntegrationStub->RequestLayout(*ResolvedConfig,
			FSurvivalStubLayoutReady::CreateUObject(this, &ThisClass::OnStubLayoutReady));
	}
	else if (!DiscoverRuntimeProviders() || !UpdateProductionLayout())
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		MatchUpdateTimer,
		this,
		&ThisClass::UpdateMatch,
		MatchUpdateIntervalSeconds,
		true);
}

bool ASurvivalGameMode::DiscoverRuntimeProviders()
{
	WorldRuntimeProvider.Reset();
	RuntimeSpawnProvider.Reset();
	bRuntimeProvidersValid = false;

	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
	{
		return false;
	}

	TArray<AActor*> WorldProviders;
	TArray<AActor*> SpawnProviders;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Candidate = *It;
		if (Candidate->GetClass()->ImplementsInterface(USurvivalWorldRuntimeInterface::StaticClass()))
		{
			WorldProviders.Add(Candidate);
		}
		if (Candidate->GetClass()->ImplementsInterface(USurvivalRuntimeSpawnInterface::StaticClass()))
		{
			SpawnProviders.Add(Candidate);
		}
	}

	if (WorldProviders.Num() != 1 || SpawnProviders.Num() != 1)
	{
		const auto DescribeProviders = [](const TArray<AActor*>& Providers)
		{
			TArray<FString> Names;
			for (const AActor* Provider : Providers)
			{
				Names.Add(Provider ? Provider->GetClass()->GetName() : TEXT("None"));
			}
			return FString::Join(Names, TEXT(", "));
		};
		UE_LOG(LogTemp, Error, TEXT("Survival provider discovery failed: World=%d [%s], Spawn=%d [%s]."),
			WorldProviders.Num(), *DescribeProviders(WorldProviders), SpawnProviders.Num(), *DescribeProviders(SpawnProviders));
		return false;
	}

	WorldRuntimeProvider = WorldProviders[0];
	RuntimeSpawnProvider = SpawnProviders[0];
	bRuntimeProvidersValid = true;
	return true;
}

bool ASurvivalGameMode::HasValidRuntimeProviders() const
{
	return bRuntimeProvidersValid && WorldRuntimeProvider.IsValid() && RuntimeSpawnProvider.IsValid();
}

bool ASurvivalGameMode::GetProductionLayoutSnapshot(FSurvivalWorldRuntimeSnapshot& OutSnapshot) const
{
	if (!HasValidRuntimeProviders())
	{
		return false;
	}

	OutSnapshot = ISurvivalWorldRuntimeInterface::Execute_GetWorldRuntimeSnapshot(WorldRuntimeProvider.Get());
	return true;
}

bool ASurvivalGameMode::UpdateProductionLayout()
{
	ASurvivalGameState* SurvivalGameState = GetSurvivalGameState();
	FSurvivalWorldRuntimeSnapshot Snapshot;
	if (!SurvivalGameState || !GetProductionLayoutSnapshot(Snapshot))
	{
		return false;
	}

	SurvivalGameState->SetUnlockedRoomCount(Snapshot.MaterializedRoomCount);
	switch (Snapshot.LayoutStatus)
	{
	case ESurvivalWorldLayoutStatus::NotRequested:
		SurvivalGameState->SetLayoutStatus(false, true);
		if (!bLayoutRequestIssued)
		{
			bLayoutRequestIssued = true;
			if (!ISurvivalWorldRuntimeInterface::Execute_RequestGenerateInitialLayout(WorldRuntimeProvider.Get(), ResolvedConfig))
			{
				UE_LOG(LogTemp, Error, TEXT("Survival World provider rejected the initial layout request."));
			}
		}
		return true;

	case ESurvivalWorldLayoutStatus::Generating:
		SurvivalGameState->SetLayoutStatus(false, true);
		return true;

	case ESurvivalWorldLayoutStatus::Succeeded:
		SurvivalGameState->SetLayoutStatus(true, true);
		return true;

	case ESurvivalWorldLayoutStatus::Failed:
		SurvivalGameState->SetLayoutStatus(false, true);
		if (!bLayoutFailureLogged)
		{
			UE_LOG(LogTemp, Error, TEXT("Survival World layout failed: %s"), *Snapshot.FailureReason);
			bLayoutFailureLogged = true;
		}
		return false;
	}

	return false;
}

bool ASurvivalGameMode::AdvanceWorldToPhase(const FSurvivalPhaseDefinition& Phase, FSurvivalWorldRuntimeSnapshot& OutSnapshot)
{
	if (!GetProductionLayoutSnapshot(OutSnapshot) || OutSnapshot.LayoutStatus != ESurvivalWorldLayoutStatus::Succeeded)
	{
		return false;
	}

	if (Phase.PhaseIndex > OutSnapshot.CurrentUnlockedPhaseIndex
		&& !ISurvivalWorldRuntimeInterface::Execute_RequestAdvanceToPhase(WorldRuntimeProvider.Get(), Phase.PhaseIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("Survival World provider rejected phase advance to %d."), Phase.PhaseIndex);
		return false;
	}

	if (!GetProductionLayoutSnapshot(OutSnapshot)
		|| OutSnapshot.LayoutStatus != ESurvivalWorldLayoutStatus::Succeeded
		|| OutSnapshot.CurrentUnlockedPhaseIndex < Phase.PhaseIndex)
	{
		UE_LOG(LogTemp, Error, TEXT("Survival World provider did not confirm phase %d after advance."), Phase.PhaseIndex);
		return false;
	}

	return true;
}

bool ASurvivalGameMode::SelectEnabledAnchor(FGameplayTag AnchorTag, FSurvivalAnchorView& OutAnchor)
{
	if (!HasValidRuntimeProviders() || !AnchorTag.IsValid())
	{
		return false;
	}

	TArray<FSurvivalAnchorView> Anchors;
	ISurvivalWorldRuntimeInterface::Execute_GetAnchorsByTag(WorldRuntimeProvider.Get(), AnchorTag, Anchors);
	Anchors.RemoveAll([](const FSurvivalAnchorView& Anchor)
	{
		return !Anchor.bEnabled || !Anchor.RoomHandle.IsValid();
	});
	Anchors.Sort([](const FSurvivalAnchorView& Left, const FSurvivalAnchorView& Right)
	{
		if (Left.RoomHandle.Value != Right.RoomHandle.Value)
		{
			return Left.RoomHandle.Value < Right.RoomHandle.Value;
		}
		const FVector LeftLocation = Left.Transform.GetLocation();
		const FVector RightLocation = Right.Transform.GetLocation();
		if (LeftLocation.X != RightLocation.X)
		{
			return LeftLocation.X < RightLocation.X;
		}
		if (LeftLocation.Y != RightLocation.Y)
		{
			return LeftLocation.Y < RightLocation.Y;
		}
		return LeftLocation.Z < RightLocation.Z;
	});
	if (Anchors.IsEmpty())
	{
		return false;
	}

	OutAnchor = Anchors[MatchRandomStream.RandRange(0, Anchors.Num() - 1)];
	return true;
}

bool ASurvivalGameMode::TrySpawnDirectedResource()
{
	TArray<FGameplayTag> ValidTags;
	for (const FGameplayTag ItemTag : ResourceItemTags)
	{
		if (ItemTag.IsValid())
		{
			ValidTags.Add(ItemTag);
		}
	}
	ValidTags.Sort([](const FGameplayTag Left, const FGameplayTag Right)
	{
		return Left.GetTagName().LexicalLess(Right.GetTagName());
	});
	if (ValidTags.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Survival resource director has no valid ItemTags configured."));
		return false;
	}

	FSurvivalAnchorView Anchor;
	if (!SelectEnabledAnchor(LG::SurvivalTags::Anchor_Resource, Anchor))
	{
		return false;
	}

	const FGameplayTag SelectedItemTag = ValidTags[MatchRandomStream.RandRange(0, ValidTags.Num() - 1)];
	FSurvivalResourceSpawnRequest Request;
	Request.ItemTag = SelectedItemTag;
	Request.Quantity = GetResourceSpawnQuantityForTag(SelectedItemTag);
	Request.SpawnTransform = Anchor.Transform;
	Request.RoomHandle = Anchor.RoomHandle;
	const FSurvivalRuntimeSpawnResult Result =
		ISurvivalRuntimeSpawnInterface::Execute_TrySpawnResource(RuntimeSpawnProvider.Get(), Request);
	if (Result.bSucceeded && Result.ResultCode == ESurvivalRuntimeSpawnResultCode::Succeeded && IsValid(Result.SpawnedActor))
	{
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("Survival resource spawn failed (%d): %s"),
		static_cast<uint8>(Result.ResultCode), *Result.FailureReason);
	return false;
}

int32 ASurvivalGameMode::GetResourceSpawnQuantityForTag(FGameplayTag ItemTag) const
{
	return ItemTag.IsValid() && ItemTag.MatchesTag(LG::SurvivalTags::Item_Ammo)
		? AmmoResourceSpawnQuantity
		: ResourceSpawnQuantity;
}

bool ASurvivalGameMode::TrySpawnDirectedEnemy(const FSurvivalPhaseDefinition& Phase)
{
	PruneAliveEnemyActors();
	if (AliveEnemyActors.Num() >= Phase.MaxAliveEnemies)
	{
		return false;
	}

	FSurvivalAnchorView Anchor;
	if (!SelectEnabledAnchor(LG::SurvivalTags::Anchor_Enemy, Anchor))
	{
		return false;
	}

	FSurvivalEnemySpawnRequest Request;
	Request.EnemyArchetypeTag = EnemyArchetypeTag;
	Request.SpawnTransform = Anchor.Transform;
	Request.RoomHandle = Anchor.RoomHandle;
	Request.DifficultyMultiplier = FMath::Max(1.0f, FMath::Max(Phase.HungerDrainMultiplier, Phase.ThirstDrainMultiplier));
	const FSurvivalRuntimeSpawnResult Result =
		ISurvivalRuntimeSpawnInterface::Execute_TrySpawnEnemy(RuntimeSpawnProvider.Get(), Request);
	if (Result.bSucceeded && Result.ResultCode == ESurvivalRuntimeSpawnResultCode::Succeeded && IsValid(Result.SpawnedActor))
	{
		AliveEnemyActors.Add(Result.SpawnedActor);
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("Survival enemy spawn failed (%d): %s"),
		static_cast<uint8>(Result.ResultCode), *Result.FailureReason);
	return false;
}

void ASurvivalGameMode::PruneAliveEnemyActors()
{
	for (auto It = AliveEnemyActors.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

bool ASurvivalGameMode::ResolveAndValidateConfig()
{
	ResolvedConfig = ModeConfig.LoadSynchronous();
	if (!ResolvedConfig && bAllowDevelopmentFallbackConfig)
	{
		ResolvedConfig = BuildDevelopmentFallbackConfig();
		UE_LOG(LogTemp, Warning,
			TEXT("TEMP_SURVIVAL_INTEGRATION_STUB: using the development fallback Survival mode config."));
	}

	FString FailureReason;
	bConfigurationValid = ResolvedConfig && ValidateConfig(RuntimePhases, FailureReason);
	if (!bConfigurationValid)
	{
		RuntimePhases.Reset();
		UE_LOG(LogTemp, Error, TEXT("Invalid Survival mode config: %s"), *FailureReason);
	}

	return bConfigurationValid;
}

USurvivalModeConfig* ASurvivalGameMode::BuildDevelopmentFallbackConfig()
{
	USurvivalModeConfig* FallbackConfig = NewObject<USurvivalModeConfig>(this, TEXT("DevelopmentFallbackSurvivalModeConfig"));
	FallbackConfig->RandomSeed = 1337;
	FallbackConfig->MaxRoomCount = 2;
	FallbackConfig->RespawnEnergyCost = 1;
	FallbackConfig->RespawnDelaySeconds = 15.0f;

	const auto AddPhase = [FallbackConfig](
		int32 PhaseIndex,
		float StartTimeSeconds,
		int32 RoomsToUnlock,
		int32 ResourceBudget,
		int32 EnemyBudget,
		int32 MaxAliveEnemies,
		float HungerDrainMultiplier,
		float ThirstDrainMultiplier,
		float NormalRoomWeight,
		float MonsterRoomWeight,
		float HighResourceRoomWeight)
	{
		FSurvivalPhaseDefinition& Phase = FallbackConfig->Phases.AddDefaulted_GetRef();
		Phase.PhaseIndex = PhaseIndex;
		Phase.StartTimeSeconds = StartTimeSeconds;
		Phase.RoomsToUnlock = RoomsToUnlock;
		Phase.ResourceBudget = ResourceBudget;
		Phase.EnemyBudget = EnemyBudget;
		Phase.MaxAliveEnemies = MaxAliveEnemies;
		Phase.HungerDrainMultiplier = HungerDrainMultiplier;
		Phase.ThirstDrainMultiplier = ThirstDrainMultiplier;
		Phase.RoomTypeWeights.Add(LG::SurvivalTags::Room_Type_Normal, NormalRoomWeight);
		Phase.RoomTypeWeights.Add(LG::SurvivalTags::Room_Type_Monster, MonsterRoomWeight);
		Phase.RoomTypeWeights.Add(LG::SurvivalTags::Room_Type_HighResource, HighResourceRoomWeight);
	};

	AddPhase(0, 0.0f, 2, 0, 0, 0, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f);
	AddPhase(1, 120.0f, 5, 8, 6, 3, 1.1f, 1.1f, 0.60f, 0.25f, 0.15f);
	AddPhase(2, 300.0f, 9, 6, 12, 6, 1.3f, 1.3f, 0.40f, 0.40f, 0.20f);
	AddPhase(3, 600.0f, 12, 4, 18, 9, 1.6f, 1.6f, 0.30f, 0.50f, 0.20f);
	return FallbackConfig;
}

bool ASurvivalGameMode::ValidateConfig(
	TArray<FSurvivalPhaseDefinition>& OutSortedPhases,
	FString& OutFailureReason) const
{
	if (!ResolvedConfig)
	{
		OutFailureReason = TEXT("No USurvivalModeConfig was assigned.");
		return false;
	}

	if (ResolvedConfig->MaxRoomCount < 2)
	{
		OutFailureReason = TEXT("MaxRoomCount must be at least two team-start rooms.");
		return false;
	}
	if (ResolvedConfig->MinTeamStartGraphDistance < 0)
	{
		OutFailureReason = TEXT("MinTeamStartGraphDistance cannot be negative.");
		return false;
	}

	OutSortedPhases = ResolvedConfig->Phases;
	if (OutSortedPhases.IsEmpty())
	{
		OutFailureReason = TEXT("At least one difficulty phase is required.");
		return false;
	}

	OutSortedPhases.Sort([](const FSurvivalPhaseDefinition& Left, const FSurvivalPhaseDefinition& Right)
	{
		return Left.StartTimeSeconds < Right.StartTimeSeconds;
	});

	if (!FMath::IsNearlyZero(OutSortedPhases[0].StartTimeSeconds))
	{
		OutFailureReason = TEXT("The first difficulty phase must start at zero seconds.");
		return false;
	}

	TSet<int32> PhaseIds;
	int32 PreviousRoomsToUnlock = 0;
	float PreviousStartTimeSeconds = -1.0f;
	for (const FSurvivalPhaseDefinition& Phase : OutSortedPhases)
	{
		if (Phase.StartTimeSeconds < 0.0f || Phase.StartTimeSeconds <= PreviousStartTimeSeconds)
		{
			OutFailureReason = TEXT("Difficulty phase start times must be strictly increasing and non-negative.");
			return false;
		}

		if (PhaseIds.Contains(Phase.PhaseIndex))
		{
			OutFailureReason = TEXT("Difficulty phase indexes must be unique.");
			return false;
		}

		if (Phase.RoomsToUnlock < PreviousRoomsToUnlock || Phase.RoomsToUnlock > ResolvedConfig->MaxRoomCount)
		{
			OutFailureReason = TEXT("RoomsToUnlock must be a cumulative, non-decreasing target within MaxRoomCount.");
			return false;
		}

		for (const TPair<FGameplayTag, float>& Weight : Phase.RoomTypeWeights)
		{
			if (Weight.Value < 0.0f)
			{
				OutFailureReason = TEXT("Room type weights cannot be negative.");
				return false;
			}
		}

		PhaseIds.Add(Phase.PhaseIndex);
		PreviousRoomsToUnlock = Phase.RoomsToUnlock;
		PreviousStartTimeSeconds = Phase.StartTimeSeconds;
	}

	if (OutSortedPhases[0].RoomsToUnlock < 2)
	{
		OutFailureReason = TEXT("The first phase must unlock both team-start rooms.");
		return false;
	}

	return true;
}

void ASurvivalGameMode::OnStubLayoutReady(bool bSucceeded)
{
	ASurvivalGameState* SurvivalGameState = GetSurvivalGameState();
	if (!SurvivalGameState)
	{
		return;
	}

	if (!bSucceeded)
	{
		UE_LOG(LogTemp, Error, TEXT("TEMP_SURVIVAL_INTEGRATION_STUB: failed to prepare a valid layout."));
		SurvivalGameState->SetLayoutStatus(false, true);
		return;
	}

	SurvivalGameState->SetLayoutStatus(true, true);
	TryStartCountdown();
}

void ASurvivalGameMode::UpdateMatch()
{
	if (!HasAuthority())
	{
		return;
	}

	PruneInvalidControllers();
	ASurvivalGameState* SurvivalGameState = GetSurvivalGameState();
	if (!SurvivalGameState || !bConfigurationValid)
	{
		return;
	}

	const float ServerTimeSeconds = GetServerTimeSeconds();
	switch (SurvivalGameState->GetSurvivalMatchPhase())
	{
	case ESurvivalMatchPhase::WaitingForLayout:
		if (!bUseDevelopmentIntegrationStubs && !UpdateProductionLayout())
		{
			break;
		}
		TryStartCountdown();
		break;

	case ESurvivalMatchPhase::Countdown:
		if (!HasEligibleTeam(ETeamType::ETT_Police) || !HasEligibleTeam(ETeamType::ETT_Bandit))
		{
			ReturnToWaitingForTeams();
		}
		else if (ServerTimeSeconds >= SurvivalGameState->GetPhaseStartServerTime() + CountdownDurationSeconds)
		{
			StartSurvival();
		}
		break;

	case ESurvivalMatchPhase::InProgress:
		UpdateDifficultyPhase(ServerTimeSeconds - SurvivalGameState->GetSurvivalStartServerTime());
		AdvanceDirectors(ServerTimeSeconds);
		ProcessRespawnQueues(ServerTimeSeconds);
		if (bOutcomeEvaluationPending)
		{
			bOutcomeEvaluationPending = false;
			EvaluateOutcome();
		}
		break;

	case ESurvivalMatchPhase::PostMatch:
		break;
	}

	RefreshReplicatedTeamStates();
}

void ASurvivalGameMode::TryStartCountdown()
{
	ASurvivalGameState* SurvivalGameState = GetSurvivalGameState();
	if (!SurvivalGameState || !SurvivalGameState->IsLayoutReady()
		|| SurvivalGameState->GetSurvivalMatchPhase() != ESurvivalMatchPhase::WaitingForLayout)
	{
		return;
	}

	const bool bBothTeamsReady = HasEligibleTeam(ETeamType::ETT_Police) && HasEligibleTeam(ETeamType::ETT_Bandit);
	SurvivalGameState->SetLayoutStatus(true, !bBothTeamsReady);
	if (!bBothTeamsReady)
	{
		return;
	}

	for (const TWeakObjectPtr<APlayerController>& WeakController : RegisteredControllers)
	{
		if (APlayerController* Controller = WeakController.Get())
		{
			if (ASurvivalPlayerState* PlayerState = Controller->GetPlayerState<ASurvivalPlayerState>())
			{
				PlayerState->SetMatchParticipant(IsValidPlayerTeam(PlayerState->GetTeamType()));
				PlayerState->SetLifeState(ESurvivalLifeState::Spectating, GetServerTimeSeconds());
			}
		}
	}

	SurvivalGameState->SetMatchPhase(ESurvivalMatchPhase::Countdown, GetServerTimeSeconds());
	SurvivalGameState->SetCountdownEndServerTime(GetServerTimeSeconds() + CountdownDurationSeconds);
	RefreshReplicatedTeamStates();
}

void ASurvivalGameMode::ReturnToWaitingForTeams()
{
	ASurvivalGameState* SurvivalGameState = GetSurvivalGameState();
	if (!SurvivalGameState)
	{
		return;
	}

	for (const TWeakObjectPtr<APlayerController>& WeakController : RegisteredControllers)
	{
		if (APlayerController* Controller = WeakController.Get())
		{
			if (ASurvivalPlayerState* PlayerState = Controller->GetPlayerState<ASurvivalPlayerState>())
			{
				PlayerState->SetMatchParticipant(false);
				PlayerState->SetLifeState(ESurvivalLifeState::Spectating, GetServerTimeSeconds());
			}
		}
	}

	SurvivalGameState->SetMatchPhase(ESurvivalMatchPhase::WaitingForLayout, GetServerTimeSeconds());
	SurvivalGameState->SetCountdownEndServerTime(0.0f);
	SurvivalGameState->SetLayoutStatus(SurvivalGameState->IsLayoutReady(), true);
	RefreshReplicatedTeamStates();
}

void ASurvivalGameMode::StartSurvival()
{
	ASurvivalGameState* SurvivalGameState = GetSurvivalGameState();
	if (!SurvivalGameState || RuntimePhases.IsEmpty())
	{
		return;
	}

	const float ServerTimeSeconds = GetServerTimeSeconds();
	SurvivalGameState->SetMatchPhase(ESurvivalMatchPhase::InProgress, ServerTimeSeconds);
	SurvivalGameState->SetSurvivalStartServerTime(ServerTimeSeconds);
	SurvivalGameState->SetCountdownEndServerTime(0.0f);
	SurvivalGameState->SetLayoutStatus(true, false);
	SurvivalGameState->SetOutcome(ESurvivalMatchOutcome::None);
	ApplyDifficultyPhase(0);

	for (const TWeakObjectPtr<APlayerController>& WeakController : RegisteredControllers)
	{
		APlayerController* Controller = WeakController.Get();
		ASurvivalPlayerState* PlayerState = Controller ? Controller->GetPlayerState<ASurvivalPlayerState>() : nullptr;
		if (!Controller || !PlayerState || !PlayerState->IsMatchParticipant())
		{
			continue;
		}

		if (RestartPlayerForTeam(Controller, PlayerState->GetTeamType()))
		{
			PlayerState->ClearRespawnState();
			PlayerState->SetLifeState(ESurvivalLifeState::Alive, ServerTimeSeconds);
		}
		else
		{
			PlayerState->SetLifeState(ESurvivalLifeState::Spectating, ServerTimeSeconds);
		}
	}

	RefreshReplicatedTeamStates();
	ScheduleOutcomeEvaluation();
}

void ASurvivalGameMode::UpdateDifficultyPhase(float SurvivalElapsedSeconds)
{
	int32 NewPhaseArrayIndex = 0;
	for (int32 PhaseArrayIndex = 0; PhaseArrayIndex < RuntimePhases.Num(); ++PhaseArrayIndex)
	{
		if (RuntimePhases[PhaseArrayIndex].StartTimeSeconds <= SurvivalElapsedSeconds)
		{
			NewPhaseArrayIndex = PhaseArrayIndex;
		}
		else
		{
			break;
		}
	}

	if (NewPhaseArrayIndex != ActiveSortedPhaseArrayIndex)
	{
		ApplyDifficultyPhase(NewPhaseArrayIndex);
	}
}

void ASurvivalGameMode::ApplyDifficultyPhase(int32 SortedPhaseArrayIndex)
{
	if (!RuntimePhases.IsValidIndex(SortedPhaseArrayIndex))
	{
		return;
	}

	const FSurvivalPhaseDefinition& Phase = RuntimePhases[SortedPhaseArrayIndex];
	FSurvivalWorldRuntimeSnapshot Snapshot;
	if (bUseDevelopmentIntegrationStubs)
	{
		if (!IntegrationStub)
		{
			return;
		}
		Snapshot.CurrentUnlockedPhaseIndex = Phase.PhaseIndex;
		Snapshot.MaterializedRoomCount = IntegrationStub->UnlockRoomsTo(Phase.RoomsToUnlock, Phase.RoomTypeWeights);
	}
	else if (!AdvanceWorldToPhase(Phase, Snapshot))
	{
		return;
	}

	ActiveSortedPhaseArrayIndex = SortedPhaseArrayIndex;
	const float ServerTimeSeconds = GetServerTimeSeconds();
	const float BudgetWindowSeconds = GetCurrentPhaseBudgetWindowSeconds(SortedPhaseArrayIndex);

	ResourceDirector.RemainingBudget = Phase.ResourceBudget;
	ResourceDirector.AttemptIntervalSeconds = FMath::Max(0.25f, BudgetWindowSeconds / FMath::Max(1, Phase.ResourceBudget));
	ResourceDirector.NextAttemptServerTime = ServerTimeSeconds;

	EnemyDirector.RemainingBudget = Phase.EnemyBudget;
	EnemyDirector.AttemptIntervalSeconds = FMath::Max(0.25f, BudgetWindowSeconds / FMath::Max(1, Phase.EnemyBudget));
	EnemyDirector.NextAttemptServerTime = ServerTimeSeconds;

	if (ASurvivalGameState* SurvivalGameState = GetSurvivalGameState())
	{
		SurvivalGameState->SetActiveDifficultyPhaseIndex(Phase.PhaseIndex);
		SurvivalGameState->SetUnlockedRoomCount(Snapshot.MaterializedRoomCount);
	}

	RefreshDirectorSnapshot();
}

void ASurvivalGameMode::AdvanceDirectors(float ServerTimeSeconds)
{
	if (!RuntimePhases.IsValidIndex(ActiveSortedPhaseArrayIndex)
		|| (!bUseDevelopmentIntegrationStubs && !HasValidRuntimeProviders()))
	{
		return;
	}

	const FSurvivalPhaseDefinition& Phase = RuntimePhases[ActiveSortedPhaseArrayIndex];
	const auto AdvanceDirector = [ServerTimeSeconds](FDirectorRuntimeState& Director, const TFunctionRef<bool()>& RequestSpawn)
	{
		int32 AttemptsThisUpdate = 0;
		while (Director.RemainingBudget > 0
			&& ServerTimeSeconds >= Director.NextAttemptServerTime
			&& AttemptsThisUpdate++ < 16)
		{
			if (RequestSpawn())
			{
				--Director.RemainingBudget;
			}

			Director.NextAttemptServerTime = FMath::Max(
				Director.NextAttemptServerTime + Director.AttemptIntervalSeconds,
				ServerTimeSeconds + 0.01f);
		}
	};

	AdvanceDirector(ResourceDirector, [this]()
	{
		return bUseDevelopmentIntegrationStubs
			? IntegrationStub && IntegrationStub->TrySpawnResource()
			: TrySpawnDirectedResource();
	});

	AdvanceDirector(EnemyDirector, [this, &Phase]()
	{
		return bUseDevelopmentIntegrationStubs
			? IntegrationStub && IntegrationStub->TrySpawnEnemy(Phase.MaxAliveEnemies)
			: TrySpawnDirectedEnemy(Phase);
	});

	RefreshDirectorSnapshot();
}

void ASurvivalGameMode::RefreshDirectorSnapshot()
{
	ASurvivalGameState* SurvivalGameState = GetSurvivalGameState();
	if (!SurvivalGameState)
	{
		return;
	}

	FSurvivalDirectorSnapshot Snapshot;
	Snapshot.ResourceBudgetRemaining = ResourceDirector.RemainingBudget;
	Snapshot.EnemyBudgetRemaining = EnemyDirector.RemainingBudget;
	PruneAliveEnemyActors();
	Snapshot.AliveEnemies = bUseDevelopmentIntegrationStubs && IntegrationStub
		? IntegrationStub->GetAliveEnemyCount()
		: AliveEnemyActors.Num();
	SurvivalGameState->SetDirectorSnapshot(Snapshot);
}

void ASurvivalGameMode::RegisterController(APlayerController* Controller)
{
	if (!HasAuthority() || !Controller)
	{
		return;
	}

	RegisteredControllers.Add(Controller);
	ASurvivalPlayerState* PlayerState = Controller->GetPlayerState<ASurvivalPlayerState>();
	ASurvivalGameState* SurvivalGameState = GetSurvivalGameState();
	if (!PlayerState || !SurvivalGameState)
	{
		return;
	}

	AssignFallbackTeamIfNeeded(*PlayerState);

	const ESurvivalMatchPhase MatchPhase = SurvivalGameState->GetSurvivalMatchPhase();
	const bool bCanJoinMatch = MatchPhase == ESurvivalMatchPhase::WaitingForLayout || MatchPhase == ESurvivalMatchPhase::Countdown;
	PlayerState->SetMatchParticipant(bCanJoinMatch && IsValidPlayerTeam(PlayerState->GetTeamType()));
	PlayerState->SetLifeState(ESurvivalLifeState::Spectating, GetServerTimeSeconds());

	if (MatchPhase == ESurvivalMatchPhase::InProgress || MatchPhase == ESurvivalMatchPhase::PostMatch)
	{
		PlayerState->SetMatchParticipant(false);
	}

	RefreshReplicatedTeamStates();
	TryStartCountdown();
}

void ASurvivalGameMode::UnregisterController(APlayerController* Controller)
{
	RegisteredControllers.Remove(Controller);
}

void ASurvivalGameMode::PruneInvalidControllers()
{
	for (auto It = RegisteredControllers.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

void ASurvivalGameMode::EnterTemporarySpectatorState(APlayerController* Controller)
{
	if (!HasAuthority() || !Controller)
	{
		return;
	}

	DestroyExistingPlayerPawn(Controller);

	if (APlayerState* PlayerState = Controller->PlayerState)
	{
		// StartSpectatingOnly sets this permanently and makes MustSpectate reject RestartPlayerAtTransform.
		PlayerState->SetIsOnlyASpectator(false);
		PlayerState->SetIsSpectator(true);
	}

	Controller->ChangeState(NAME_Spectating);
	Controller->ClientGotoState(NAME_Spectating);
}

void ASurvivalGameMode::DestroyExistingPlayerPawn(APlayerController* Controller)
{
	if (!HasAuthority() || !Controller)
	{
		return;
	}

	APawn* ExistingPawn = Controller->GetPawn();
	if (!ExistingPawn)
	{
		return;
	}

	Controller->UnPossess();
	ExistingPawn->Destroy();
}

void ASurvivalGameMode::AssignFallbackTeamIfNeeded(ASurvivalPlayerState& PlayerState)
{
	if (PlayerState.GetTeamType() != ETeamType::ETT_None)
	{
		return;
	}

	int32 PoliceCount = 0;
	int32 BanditCount = 0;
	for (const TWeakObjectPtr<APlayerController>& WeakController : RegisteredControllers)
	{
		const APlayerController* RegisteredController = WeakController.Get();
		const ASurvivalPlayerState* RegisteredPlayerState = RegisteredController
			? RegisteredController->GetPlayerState<ASurvivalPlayerState>()
			: nullptr;
		if (!RegisteredPlayerState || RegisteredPlayerState == &PlayerState)
		{
			continue;
		}

		if (RegisteredPlayerState->GetTeamType() == ETeamType::ETT_Police)
		{
			++PoliceCount;
		}
		else if (RegisteredPlayerState->GetTeamType() == ETeamType::ETT_Bandit)
		{
			++BanditCount;
		}
	}

	PlayerState.AssignServerMatchTeam(DetermineBalancedFallbackTeam(PoliceCount, BanditCount));
}

ETeamType ASurvivalGameMode::DetermineBalancedFallbackTeam(int32 PoliceCount, int32 BanditCount)
{
	ETeamType AssignedTeam = ETeamType::ETT_Police;
	if (PoliceCount < BanditCount)
	{
		AssignedTeam = ETeamType::ETT_Police;
	}
	else if (BanditCount < PoliceCount)
	{
		AssignedTeam = ETeamType::ETT_Bandit;
	}
	else
	{
		AssignedTeam = bNextBalancedTeamIsPolice ? ETeamType::ETT_Police : ETeamType::ETT_Bandit;
		bNextBalancedTeamIsPolice = !bNextBalancedTeamIsPolice;
	}
	return AssignedTeam;
}

bool ASurvivalGameMode::HasEligibleTeam(ETeamType TeamType) const
{
	for (const TWeakObjectPtr<APlayerController>& WeakController : RegisteredControllers)
	{
		const APlayerController* Controller = WeakController.Get();
		const ASurvivalPlayerState* PlayerState = Controller ? Controller->GetPlayerState<ASurvivalPlayerState>() : nullptr;
		if (PlayerState && PlayerState->GetTeamType() == TeamType)
		{
			return true;
		}
	}

	return false;
}

bool ASurvivalGameMode::IsValidPlayerTeam(ETeamType TeamType) const
{
	return TeamType == ETeamType::ETT_Police || TeamType == ETeamType::ETT_Bandit;
}

void ASurvivalGameMode::EnqueueRespawn(APlayerController* Controller, ASurvivalPlayerState* PlayerState)
{
	if (!Controller || !PlayerState)
	{
		return;
	}

	FTeamRuntimeState* TeamState = GetTeamRuntimeState(PlayerState->GetTeamType());
	if (!TeamState || TeamState->bEliminated)
	{
		PlayerState->SetLifeState(ESurvivalLifeState::Eliminated, GetServerTimeSeconds());
		return;
	}

	RemoveFromRespawnQueue(PlayerState, true);
	FRespawnQueueEntry& Entry = TeamState->RespawnQueue.AddDefaulted_GetRef();
	Entry.Controller = Controller;
	Entry.PlayerState = PlayerState;
	ReserveRespawnEnergy(*TeamState, GetServerTimeSeconds());
	RefreshRespawnQueueState(*TeamState);
}

void ASurvivalGameMode::RemoveFromRespawnQueue(ASurvivalPlayerState* PlayerState, bool bRefundReservedEnergy)
{
	if (!PlayerState)
	{
		return;
	}

	FTeamRuntimeState* TeamState = GetTeamRuntimeState(PlayerState->GetTeamType());
	if (!TeamState)
	{
		return;
	}

	for (int32 EntryIndex = TeamState->RespawnQueue.Num() - 1; EntryIndex >= 0; --EntryIndex)
	{
		const FRespawnQueueEntry& Entry = TeamState->RespawnQueue[EntryIndex];
		if (Entry.PlayerState.Get() == PlayerState)
		{
			if (bRefundReservedEnergy && Entry.bEnergyReserved)
			{
				TeamState->RespawnEnergy += GetCurrentRespawnEnergyCost();
			}

			TeamState->RespawnQueue.RemoveAt(EntryIndex);
		}
	}

	PlayerState->ClearRespawnState();
	RefreshRespawnQueueState(*TeamState);
}

void ASurvivalGameMode::ReserveRespawnEnergy(FTeamRuntimeState& TeamState, float ServerTimeSeconds)
{
	const int32 RespawnEnergyCost = GetCurrentRespawnEnergyCost();
	for (FRespawnQueueEntry& Entry : TeamState.RespawnQueue)
	{
		if (Entry.bEnergyReserved || TeamState.RespawnEnergy < RespawnEnergyCost)
		{
			continue;
		}

		TeamState.RespawnEnergy -= RespawnEnergyCost;
		Entry.bEnergyReserved = true;
		Entry.RespawnReadyServerTime = ServerTimeSeconds + GetCurrentRespawnDelaySeconds();
		UE_LOG(LogSurvivalMatchRespawn, Display,
			TEXT("Survival respawn energy reserved: player=%s team=%d ready=%.2f remaining-energy=%d."),
			*GetNameSafe(Entry.PlayerState.Get()), static_cast<uint8>(TeamState.TeamType),
			Entry.RespawnReadyServerTime, TeamState.RespawnEnergy);
	}
}

void ASurvivalGameMode::ProcessRespawnQueues(float ServerTimeSeconds)
{
	for (FTeamRuntimeState* TeamState : { &PoliceTeamState, &BanditTeamState })
	{
		if (!TeamState || TeamState->bEliminated)
		{
			continue;
		}

		ReserveRespawnEnergy(*TeamState, ServerTimeSeconds);
		while (!TeamState->RespawnQueue.IsEmpty())
		{
			FRespawnQueueEntry& Entry = TeamState->RespawnQueue[0];
			ASurvivalPlayerState* PlayerState = Entry.PlayerState.Get();
			APlayerController* Controller = Entry.Controller.Get();
			if (!PlayerState || !Controller)
			{
				if (Entry.bEnergyReserved)
				{
					TeamState->RespawnEnergy += GetCurrentRespawnEnergyCost();
				}
				TeamState->RespawnQueue.RemoveAt(0);
				continue;
			}

			if (!Entry.bEnergyReserved || Entry.RespawnReadyServerTime > ServerTimeSeconds)
			{
				break;
			}

			PlayerState->SetLifeState(ESurvivalLifeState::Respawning, ServerTimeSeconds);
			UE_LOG(LogSurvivalMatchRespawn, Display,
				TEXT("Survival respawn attempt: player=%s team=%d queue=%d."),
				*GetNameSafe(PlayerState), static_cast<uint8>(TeamState->TeamType), 1);
			if (RestartPlayerForTeam(Controller, TeamState->TeamType))
			{
				TeamState->RespawnQueue.RemoveAt(0);
				PlayerState->ClearRespawnState();
				PlayerState->SetLifeState(ESurvivalLifeState::Alive, ServerTimeSeconds);
				UE_LOG(LogSurvivalMatchRespawn, Display,
					TEXT("Survival respawn succeeded: player=%s team=%d pawn=%s."),
					*GetNameSafe(PlayerState), static_cast<uint8>(TeamState->TeamType),
					*GetNameSafe(Controller->GetPawn()));
				ReserveRespawnEnergy(*TeamState, ServerTimeSeconds);
				continue;
			}

			PlayerState->SetLifeState(ESurvivalLifeState::WaitingRespawn, ServerTimeSeconds);
			Entry.RespawnReadyServerTime = ServerTimeSeconds + 1.0f;
			UE_LOG(LogSurvivalMatchRespawn, Warning,
				TEXT("Survival respawn retry scheduled: player=%s team=%d next-attempt=%.2f."),
				*GetNameSafe(PlayerState), static_cast<uint8>(TeamState->TeamType), Entry.RespawnReadyServerTime);
			break;
		}

		RefreshRespawnQueueState(*TeamState);
	}
}

void ASurvivalGameMode::RefreshRespawnQueueState(FTeamRuntimeState& TeamState)
{
	for (int32 QueueIndex = 0; QueueIndex < TeamState.RespawnQueue.Num(); ++QueueIndex)
	{
		if (ASurvivalPlayerState* PlayerState = TeamState.RespawnQueue[QueueIndex].PlayerState.Get())
		{
			const FRespawnQueueEntry& Entry = TeamState.RespawnQueue[QueueIndex];
			PlayerState->SetRespawnState(QueueIndex + 1, Entry.RespawnReadyServerTime);
		}
	}
}

bool ASurvivalGameMode::RestartPlayerForTeam(APlayerController* Controller, ETeamType TeamType)
{
	if (!Controller)
	{
		return false;
	}

	// A seamless-travel or default-map pawn keeps its previous transform when reused by
	// RestartPlayerAtTransform. Survival always creates a fresh pawn at its generated anchor.
	DestroyExistingPlayerPawn(Controller);

	FTransform SpawnTransform;
	if (bUseDevelopmentIntegrationStubs)
	{
		if (!IntegrationStub || !IntegrationStub->GetTeamSpawnTransform(GetWorld(), TeamType, SpawnTransform))
		{
			return false;
		}
		RestartPlayerAtTransform(Controller, SpawnTransform);
	}
	else
	{
		if (!HasValidRuntimeProviders()
			|| !ISurvivalWorldRuntimeInterface::Execute_GetTeamPlayerStartTransform(WorldRuntimeProvider.Get(), TeamType, SpawnTransform))
		{
			UE_LOG(LogTemp, Warning, TEXT("Survival team %d has no enabled PlayerStart anchor."), static_cast<uint8>(TeamType));
			return false;
		}
		RestartPlayerAtTransform(Controller, SpawnTransform);
	}

	return Controller->GetPawn() != nullptr;
}

void ASurvivalGameMode::RefreshReplicatedTeamStates()
{
	ASurvivalGameState* SurvivalGameState = GetSurvivalGameState();
	if (!SurvivalGameState)
	{
		return;
	}

	for (FTeamRuntimeState* TeamRuntimeState : { &PoliceTeamState, &BanditTeamState })
	{
		FTeamSurvivalState ReplicatedTeamState;
		ReplicatedTeamState.TeamType = TeamRuntimeState->TeamType;
		ReplicatedTeamState.RespawnEnergy = TeamRuntimeState->RespawnEnergy;
		ReplicatedTeamState.bEliminated = TeamRuntimeState->bEliminated;

		for (const TWeakObjectPtr<APlayerController>& WeakController : RegisteredControllers)
		{
			const APlayerController* Controller = WeakController.Get();
			const ASurvivalPlayerState* PlayerState = Controller ? Controller->GetPlayerState<ASurvivalPlayerState>() : nullptr;
			if (!PlayerState || !PlayerState->IsMatchParticipant() || PlayerState->GetTeamType() != TeamRuntimeState->TeamType)
			{
				continue;
			}

			if (PlayerState->GetSurvivalLifeState() == ESurvivalLifeState::Alive)
			{
				++ReplicatedTeamState.AlivePlayers;
			}
			else if (PlayerState->GetSurvivalLifeState() == ESurvivalLifeState::WaitingRespawn
				|| PlayerState->GetSurvivalLifeState() == ESurvivalLifeState::Respawning)
			{
				++ReplicatedTeamState.WaitingPlayers;
			}
		}

		SurvivalGameState->SetTeamState(TeamRuntimeState->TeamType, ReplicatedTeamState);
	}
}

void ASurvivalGameMode::ScheduleOutcomeEvaluation()
{
	if (ASurvivalGameState* SurvivalGameState = GetSurvivalGameState())
	{
		if (SurvivalGameState->GetSurvivalMatchPhase() == ESurvivalMatchPhase::InProgress)
		{
			bOutcomeEvaluationPending = true;
		}
	}
}

void ASurvivalGameMode::EvaluateOutcome()
{
	const bool bPoliceViable = IsTeamViable(PoliceTeamState);
	const bool bBanditViable = IsTeamViable(BanditTeamState);
	PoliceTeamState.bEliminated = !bPoliceViable;
	BanditTeamState.bEliminated = !bBanditViable;
	RefreshReplicatedTeamStates();

	if (!bPoliceViable && !bBanditViable)
	{
		CompleteMatch(ETeamType::ETT_None, true);
	}
	else if (bPoliceViable != bBanditViable)
	{
		CompleteMatch(bPoliceViable ? ETeamType::ETT_Police : ETeamType::ETT_Bandit, false);
	}
}

bool ASurvivalGameMode::IsTeamViable(const FTeamRuntimeState& TeamState) const
{
	for (const TWeakObjectPtr<APlayerController>& WeakController : RegisteredControllers)
	{
		const APlayerController* Controller = WeakController.Get();
		const ASurvivalPlayerState* PlayerState = Controller ? Controller->GetPlayerState<ASurvivalPlayerState>() : nullptr;
		if (PlayerState && PlayerState->IsMatchParticipant() && PlayerState->GetTeamType() == TeamState.TeamType
			&& PlayerState->GetSurvivalLifeState() == ESurvivalLifeState::Alive)
		{
			return true;
		}
	}

	return TeamState.RespawnQueue.ContainsByPredicate([](const FRespawnQueueEntry& Entry)
	{
		return Entry.bEnergyReserved && Entry.PlayerState.IsValid() && Entry.Controller.IsValid();
	});
}

void ASurvivalGameMode::CompleteMatch(ETeamType WinningTeam, bool bDraw)
{
	ASurvivalGameState* SurvivalGameState = GetSurvivalGameState();
	if (!SurvivalGameState || SurvivalGameState->GetSurvivalMatchPhase() == ESurvivalMatchPhase::PostMatch)
	{
		return;
	}

	for (FTeamRuntimeState* TeamState : { &PoliceTeamState, &BanditTeamState })
	{
		const bool bTeamWon = !bDraw && TeamState->TeamType == WinningTeam;
		TeamState->bEliminated = !bTeamWon;
		for (const FRespawnQueueEntry& Entry : TeamState->RespawnQueue)
		{
			if (ASurvivalPlayerState* PlayerState = Entry.PlayerState.Get())
			{
				UE_LOG(LogSurvivalMatchRespawn, Display,
					TEXT("Survival match outcome cancels respawn: player=%s team=%d reserved=%d winner=%d draw=%d."),
					*GetNameSafe(PlayerState), static_cast<uint8>(TeamState->TeamType), Entry.bEnergyReserved ? 1 : 0,
					static_cast<uint8>(WinningTeam), bDraw ? 1 : 0);
				PlayerState->ClearRespawnState();
				PlayerState->SetLifeState(bTeamWon ? ESurvivalLifeState::Spectating : ESurvivalLifeState::Eliminated,
					GetServerTimeSeconds());
			}
		}
		TeamState->RespawnQueue.Reset();
	}

	SurvivalGameState->SetOutcome(bDraw
		? ESurvivalMatchOutcome::Draw
		: (WinningTeam == ETeamType::ETT_Police ? ESurvivalMatchOutcome::PoliceVictory : ESurvivalMatchOutcome::BanditVictory));
	SurvivalGameState->SetMatchPhase(ESurvivalMatchPhase::PostMatch, GetServerTimeSeconds());
	ResourceDirector.RemainingBudget = 0;
	EnemyDirector.RemainingBudget = 0;
	RefreshDirectorSnapshot();
	RefreshReplicatedTeamStates();
}

ASurvivalGameState* ASurvivalGameMode::GetSurvivalGameState() const
{
	return GetGameState<ASurvivalGameState>();
}

ASurvivalGameMode::FTeamRuntimeState* ASurvivalGameMode::GetTeamRuntimeState(ETeamType TeamType)
{
	if (TeamType == ETeamType::ETT_Police)
	{
		return &PoliceTeamState;
	}
	if (TeamType == ETeamType::ETT_Bandit)
	{
		return &BanditTeamState;
	}
	return nullptr;
}

const ASurvivalGameMode::FTeamRuntimeState* ASurvivalGameMode::GetTeamRuntimeState(ETeamType TeamType) const
{
	return const_cast<ASurvivalGameMode*>(this)->GetTeamRuntimeState(TeamType);
}

float ASurvivalGameMode::GetServerTimeSeconds() const
{
	return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
}

float ASurvivalGameMode::GetCurrentRespawnDelaySeconds() const
{
	return ResolvedConfig ? ResolvedConfig->RespawnDelaySeconds : 15.0f;
}

int32 ASurvivalGameMode::GetCurrentRespawnEnergyCost() const
{
	return ResolvedConfig ? FMath::Max(1, ResolvedConfig->RespawnEnergyCost) : 1;
}

float ASurvivalGameMode::GetCurrentPhaseBudgetWindowSeconds(int32 SortedPhaseArrayIndex) const
{
	if (RuntimePhases.IsValidIndex(SortedPhaseArrayIndex + 1))
	{
		return FMath::Max(0.25f,
			RuntimePhases[SortedPhaseArrayIndex + 1].StartTimeSeconds - RuntimePhases[SortedPhaseArrayIndex].StartTimeSeconds);
	}

	return FinalPhaseDirectorBudgetWindowSeconds;
}
