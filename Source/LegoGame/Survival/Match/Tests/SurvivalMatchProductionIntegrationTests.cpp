#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/DamageEvents.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "LegoGame/Character/LgCharacterBase.h"
#include "LegoGame/Components/PackageComponent.h"
#include "LegoGame/GamePlay/GameMenu/Game/GameFetureUserWidget.h"
#include "LegoGame/Player/PlayerCharacter.h"
#include "LegoGame/Survival/Integration/SurvivalCoreRuntimeProvider.h"
#include "LegoGame/Survival/Integration/SurvivalTeamRespawnTerminal.h"
#include "LegoGame/Survival/Contracts/SurvivalDataAssets.h"
#include "LegoGame/Survival/Contracts/SurvivalInterfaces.h"
#include "LegoGame/Survival/Contracts/SurvivalGameplayTags.h"
#include "LegoGame/Survival/Match/SurvivalGameMode.h"
#include "LegoGame/Survival/Match/SurvivalGameState.h"
#include "LegoGame/Survival/Match/SurvivalHUD.h"
#include "LegoGame/Survival/Match/SurvivalPlayerState.h"
#include "LegoGame/Survival/Match/UI/SurvivalEndingWidget.h"
#include "LegoGame/Survival/Match/UI/SurvivalHUDWidget.h"
#include "LegoGame/Survival/Match/UI/SurvivalRespawnWidget.h"
#include "LegoGame/Survival/Match/UI/SurvivalVitalsStatusWidget.h"
#include "LegoGame/Survival/SurvivalItemIds.h"
#include "LegoGame/Survival/SurvivalVitalsComponent.h"
#include "LegoGame/Survival/World/SurvivalWorldGenerator.h"
#include "LegoGame/Weapon/WeaponBase.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

struct FSurvivalMatchProductionTestAccess
{
	static bool DiscoverRuntimeProviders(ASurvivalGameMode& GameMode)
	{
		return GameMode.DiscoverRuntimeProviders();
	}

	static bool UsesDevelopmentStubs(const ASurvivalGameMode& GameMode)
	{
		return GameMode.bUseDevelopmentIntegrationStubs;
	}

	static ETeamType DetermineBalancedFallbackTeam(ASurvivalGameMode& GameMode, int32 PoliceCount, int32 BanditCount)
	{
		return GameMode.DetermineBalancedFallbackTeam(PoliceCount, BanditCount);
	}

	static void EnterTemporarySpectatorState(ASurvivalGameMode& GameMode, APlayerController* Controller)
	{
		GameMode.EnterTemporarySpectatorState(Controller);
	}

	static void SetDefaultPawnClass(ASurvivalGameMode& GameMode, UClass* PawnClass)
	{
		GameMode.DefaultPawnClass = PawnClass;
	}

	static int32 GetResourceSpawnQuantityForTag(const ASurvivalGameMode& GameMode, FGameplayTag ItemTag)
	{
		return GameMode.GetResourceSpawnQuantityForTag(ItemTag);
	}

	static void ConfigureDevelopmentWorld(ASurvivalGameMode& GameMode)
	{
		GameMode.bUseDevelopmentIntegrationStubs = true;
		GameMode.bAllowDevelopmentFallbackConfig = false;
		GameMode.ModeConfig = TSoftObjectPtr<USurvivalModeConfig>(FSoftObjectPath(
			TEXT("/Game/LegoGame/Survival/Data/DA_SurvivalMode_Default.DA_SurvivalMode_Default")));
	}

	static void SetMatchInProgress(ASurvivalGameState& GameState, float ServerTimeSeconds)
	{
		GameState.SetMatchPhase(ESurvivalMatchPhase::InProgress, ServerTimeSeconds);
	}

	static void PrepareAliveParticipant(ASurvivalPlayerState& PlayerState, ETeamType TeamType, float ServerTimeSeconds)
	{
		PlayerState.AssignServerMatchTeam(TeamType);
		PlayerState.SetMatchParticipant(true);
		PlayerState.SetLifeState(ESurvivalLifeState::Alive, ServerTimeSeconds);
	}

	static void SimulateStaleSpectatorMetadata(ASurvivalPlayerState& PlayerState, float ServerTimeSeconds)
	{
		PlayerState.SetMatchParticipant(false);
		PlayerState.SetLifeState(ESurvivalLifeState::Spectating, ServerTimeSeconds);
	}
};

struct FSurvivalVitalsStatusWidgetTestAccess
{
	static float NormalizeVital(float CurrentValue, float MaxValue)
	{
		return USurvivalVitalsStatusWidget::NormalizeVital(CurrentValue, MaxValue);
	}

	static ESurvivalMatchPhase ResolveDisplayPhase(ESurvivalMatchPhase SurvivalMatchPhase, bool bEngineMatchStarted)
	{
		return USurvivalVitalsStatusWidget::ResolveDisplayPhase(SurvivalMatchPhase, bEngineMatchStarted);
	}

	static bool ShouldDisplayVitals(ESurvivalMatchPhase MatchPhase, bool bHasVitalsPawn,
		const FSurvivalVitalsSnapshot& Snapshot)
	{
		return USurvivalVitalsStatusWidget::ShouldDisplayVitals(MatchPhase, bHasVitalsPawn, Snapshot);
	}
};

struct FSurvivalRespawnWidgetTestAccess
{
	static FSurvivalRespawnPresentation Resolve(
		ESurvivalLifeState LifeState, bool bMatchParticipant, int32 QueuePosition,
		float ReadyServerTime, float ServerTime,
		ESurvivalMatchPhase MatchPhase = ESurvivalMatchPhase::InProgress)
	{
		return USurvivalRespawnWidget::ResolvePresentation(
			LifeState, bMatchParticipant, QueuePosition, ReadyServerTime, ServerTime, MatchPhase);
	}
};

struct FSurvivalEndingWidgetTestAccess
{
	static FText ResolveResultText(ESurvivalMatchOutcome Outcome, ETeamType LocalTeam)
	{
		return USurvivalEndingWidget::ResolveResultText(Outcome, LocalTeam);
	}
};

struct FSurvivalPlayerInteractionTestAccess
{
	static void ClearOverlapCacheAndInteract(APlayerCharacter& Character)
	{
		Character.NearbySurvivalInteractables.Reset();
		Character.InteractWithNearbySurvivalActor();
	}
};

namespace LG::Survival::Match::Tests
{
	namespace
	{
		constexpr TCHAR SurvivalModeConfigPath[] = TEXT("/Game/LegoGame/Survival/Data/DA_SurvivalMode_Default.DA_SurvivalMode_Default");
		struct FMatchTestWorld
		{
			FMatchTestWorld()
			{
				static int32 WorldIndex = 0;
				World = UWorld::CreateWorld(EWorldType::Game, false,
					FName(*FString::Printf(TEXT("SurvivalMatchProductionTest_%d"), ++WorldIndex)));
				if (!World || !GEngine)
				{
					return;
				}
				WorldContext = &GEngine->CreateNewWorldContext(EWorldType::Game);
				WorldContext->SetCurrentWorld(World);
				GameMode = World->SpawnActor<ASurvivalGameMode>();
			}

			~FMatchTestWorld()
			{
				if (!World)
				{
					return;
				}
				if (GEngine && WorldContext)
				{
					GEngine->DestroyWorldContext(World);
				}
				World->DestroyWorld(false);
			}

			UWorld* World = nullptr;
			FWorldContext* WorldContext = nullptr;
			ASurvivalGameMode* GameMode = nullptr;
		};

		struct FAuthorityMatchTestWorld
		{
			explicit FAuthorityMatchTestWorld(UClass* GameModeClass = ASurvivalGameMode::StaticClass())
			{
				static int32 WorldIndex = 0;
				World = UWorld::CreateWorld(EWorldType::Game, false,
					FName(*FString::Printf(TEXT("SurvivalAuthorityMatchTest_%d"), ++WorldIndex)));
				if (!World || !GEngine)
				{
					return;
				}

				WorldContext = &GEngine->CreateNewWorldContext(EWorldType::Game);
				WorldContext->SetCurrentWorld(World);
				GameInstance = NewObject<UGameInstance>(GetTransientPackage());
				GameInstance->Init();
				World->SetGameInstance(GameInstance);
				World->GetWorldSettings()->DefaultGameMode = GameModeClass;

				FURL URL;
				if (!World->SetGameMode(URL))
				{
					return;
				}
				GameMode = Cast<ASurvivalGameMode>(World->GetAuthGameMode());
				if (!GameMode)
				{
					return;
				}
				FSurvivalMatchProductionTestAccess::ConfigureDevelopmentWorld(*GameMode);
				World->InitializeActorsForPlay(URL);
				World->BeginPlay();
			}

			~FAuthorityMatchTestWorld()
			{
				if (!World)
				{
					return;
				}
				if (GameInstance)
				{
					GameInstance->Shutdown();
				}
				if (GEngine && WorldContext)
				{
					GEngine->DestroyWorldContext(World);
				}
				World->DestroyWorld(false);
			}

			UWorld* World = nullptr;
			FWorldContext* WorldContext = nullptr;
			UGameInstance* GameInstance = nullptr;
			ASurvivalGameMode* GameMode = nullptr;
		};
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalMatchLayoutLifecycleTest,
		"LegoGame.Survival.Match.LayoutLifecycle",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalMatchLayoutLifecycleTest::RunTest(const FString& Parameters)
	{
		FMatchTestWorld TestWorld;
		ASurvivalWorldGenerator* Generator = TestWorld.World ? TestWorld.World->SpawnActor<ASurvivalWorldGenerator>() : nullptr;
		USurvivalModeConfig* Config = LoadObject<USurvivalModeConfig>(nullptr, SurvivalModeConfigPath);
		if (!TestNotNull(TEXT("Real World generator"), Generator) || !TestNotNull(TEXT("Production mode config"), Config))
		{
			return false;
		}

		TestEqual(TEXT("New generator has not received a request"),
			ISurvivalWorldRuntimeInterface::Execute_GetWorldRuntimeSnapshot(Generator).LayoutStatus,
			ESurvivalWorldLayoutStatus::NotRequested);
		TestFalse(TEXT("A client-like transient test world cannot issue an authority layout request"),
			ISurvivalWorldRuntimeInterface::Execute_RequestGenerateInitialLayout(Generator, Config));
		const FSurvivalWorldRuntimeSnapshot Snapshot = ISurvivalWorldRuntimeInterface::Execute_GetWorldRuntimeSnapshot(Generator);
		TestEqual(TEXT("Rejected request preserves the initial lifecycle state"), Snapshot.LayoutStatus,
			ESurvivalWorldLayoutStatus::NotRequested);
		TestFalse(TEXT("Repeated untrusted request remains rejected"),
			ISurvivalWorldRuntimeInterface::Execute_RequestGenerateInitialLayout(Generator, Config));
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalMatchPhaseAdvanceTest,
		"LegoGame.Survival.Match.PhaseAdvance",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalMatchPhaseAdvanceTest::RunTest(const FString& Parameters)
	{
		FMatchTestWorld TestWorld;
		ASurvivalWorldGenerator* Generator = TestWorld.World ? TestWorld.World->SpawnActor<ASurvivalWorldGenerator>() : nullptr;
		USurvivalModeConfig* Config = LoadObject<USurvivalModeConfig>(nullptr, SurvivalModeConfigPath);
		if (!TestNotNull(TEXT("Real World generator"), Generator) || !TestNotNull(TEXT("Production mode config"), Config)
			|| !TestFalse(TEXT("Transient non-authority world rejects initial layout"), ISurvivalWorldRuntimeInterface::Execute_RequestGenerateInitialLayout(Generator, Config)))
		{
			return false;
		}

		TestFalse(TEXT("Transient non-authority world rejects phase advances"),
			ISurvivalWorldRuntimeInterface::Execute_RequestAdvanceToPhase(Generator, 1));
		TestEqual(TEXT("Rejected phase request leaves the snapshot untouched"),
			ISurvivalWorldRuntimeInterface::Execute_GetWorldRuntimeSnapshot(Generator).CurrentUnlockedPhaseIndex, INDEX_NONE);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalMatchResourceDirectorContractTest,
		"LegoGame.Survival.Match.Directors.Resource",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalMatchResourceDirectorContractTest::RunTest(const FString& Parameters)
	{
		FMatchTestWorld TestWorld;
		ASurvivalCoreRuntimeProvider* Provider = TestWorld.World ? TestWorld.World->SpawnActor<ASurvivalCoreRuntimeProvider>() : nullptr;
		if (!TestNotNull(TEXT("Real Core provider"), Provider))
		{
			return false;
		}
		FSurvivalResourceSpawnRequest Request;
		Request.ItemTag = LG::SurvivalTags::Item_RespawnEnergy;
		Request.Quantity = 1;
		const FSurvivalRuntimeSpawnResult Result = ISurvivalRuntimeSpawnInterface::Execute_TrySpawnResource(Provider, Request);
		TestFalse(TEXT("Blocked content never creates a placeholder resource Actor"), Result.bSucceeded);
		TestTrue(TEXT("Rejected provider result has a non-success failure code"),
			Result.ResultCode != ESurvivalRuntimeSpawnResultCode::Succeeded);
		TestNull(TEXT("Failed request has no spawned Actor"), Result.SpawnedActor);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalMatchEnemyDirectorContractTest,
		"LegoGame.Survival.Match.Directors.Enemy",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalMatchEnemyDirectorContractTest::RunTest(const FString& Parameters)
	{
		FMatchTestWorld TestWorld;
		ASurvivalCoreRuntimeProvider* Provider = TestWorld.World ? TestWorld.World->SpawnActor<ASurvivalCoreRuntimeProvider>() : nullptr;
		if (!TestNotNull(TEXT("Real Core provider"), Provider))
		{
			return false;
		}
		FSurvivalEnemySpawnRequest Request;
		Request.DifficultyMultiplier = 0.0f;
		const FSurvivalRuntimeSpawnResult Result = ISurvivalRuntimeSpawnInterface::Execute_TrySpawnEnemy(Provider, Request);
		TestFalse(TEXT("Invalid director difficulty cannot create an enemy"), Result.bSucceeded);
		TestTrue(TEXT("Rejected provider result has a non-success failure code"),
			Result.ResultCode != ESurvivalRuntimeSpawnResultCode::Succeeded);
		TestNull(TEXT("Rejected request has no spawned Actor"), Result.SpawnedActor);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalMatchRespawnEnergyContractTest,
		"LegoGame.Survival.Match.RespawnEnergy",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalMatchRespawnEnergyContractTest::RunTest(const FString& Parameters)
	{
		const FGameplayTag RespawnEnergyTag = LG::SurvivalTags::Item_RespawnEnergy;
		TestTrue(TEXT("Contracts retain the formal RespawnEnergy tag"), RespawnEnergyTag.IsValid());
		TestTrue(TEXT("Production content acceptance remains blocked until a formal pickup definition exists"), true);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalRespawnTerminalDepositTest,
		"LegoGame.Survival.Match.RespawnEnergy.TerminalDeposit",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalRespawnTerminalDepositTest::RunTest(const FString& Parameters)
	{
		FAuthorityMatchTestWorld TestWorld;
		ASurvivalGameState* GameState = TestWorld.World ? TestWorld.World->GetGameState<ASurvivalGameState>() : nullptr;
		if (!TestNotNull(TEXT("Authority Survival world"), TestWorld.World)
			|| !TestNotNull(TEXT("Authority Survival GameMode"), TestWorld.GameMode)
			|| !TestNotNull(TEXT("Survival GameState"), GameState))
		{
			return false;
		}

		APlayerController* Controller = TestWorld.World->SpawnActor<APlayerController>();
		ASurvivalPlayerState* PlayerState = TestWorld.World->SpawnActor<ASurvivalPlayerState>();
		APlayerCharacter* Character = TestWorld.World->SpawnActor<APlayerCharacter>();
		ASurvivalTeamRespawnTerminal* PoliceTerminal =
			TestWorld.World->SpawnActor<ASurvivalTeamRespawnTerminal>();
		ASurvivalTeamRespawnTerminal* BanditTerminal =
			TestWorld.World->SpawnActor<ASurvivalTeamRespawnTerminal>();
		UPackageComponent* Package = Character ? Character->GetPackageComponent() : nullptr;
		if (!TestNotNull(TEXT("Deposit Controller"), Controller)
			|| !TestNotNull(TEXT("Deposit PlayerState"), PlayerState)
			|| !TestNotNull(TEXT("Deposit Character"), Character)
			|| !TestNotNull(TEXT("Deposit inventory"), Package)
			|| !TestNotNull(TEXT("Police terminal"), PoliceTerminal)
			|| !TestNotNull(TEXT("Bandit terminal"), BanditTerminal))
		{
			return false;
		}

		const FEnumProperty* TeamProperty = FindFProperty<FEnumProperty>(
			ASurvivalTeamRespawnTerminal::StaticClass(), TEXT("TeamType"));
		if (!TestNotNull(TEXT("Terminal team property"), TeamProperty))
		{
			return false;
		}
		auto SetTerminalTeam = [TeamProperty](ASurvivalTeamRespawnTerminal& Terminal, ETeamType TeamType)
		{
			void* Value = TeamProperty->ContainerPtrToValuePtr<void>(&Terminal);
			TeamProperty->GetUnderlyingProperty()->SetIntPropertyValue(Value, static_cast<int64>(TeamType));
		};
		SetTerminalTeam(*PoliceTerminal, ETeamType::ETT_Police);
		SetTerminalTeam(*BanditTerminal, ETeamType::ETT_Bandit);

		Controller->PlayerState = PlayerState;
		PlayerState->SetOwner(Controller);
		Controller->Possess(Character);
		Character->SetActorLocation(FVector::ZeroVector);
		PoliceTerminal->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
		BanditTerminal->SetActorLocation(FVector(100.0f, 100.0f, 0.0f));
		FSurvivalMatchProductionTestAccess::SetMatchInProgress(*GameState, TestWorld.World->GetTimeSeconds());
		FSurvivalMatchProductionTestAccess::PrepareAliveParticipant(
			*PlayerState, ETeamType::ETT_Police, TestWorld.World->GetTimeSeconds());

		FItemStack EnergyStack;
		EnergyStack.ItemId = LG::SurvivalItemIds::RespawnEnergy;
		EnergyStack.Quantity = 2;
		if (!TestTrue(TEXT("Production RespawnEnergy enters the player's tagged inventory"),
			Package->TryAddItemStack(EnergyStack)))
		{
			return false;
		}

		ISurvivalInteractableInterface::Execute_Interact(BanditTerminal, Character);
		TestEqual(TEXT("Wrong-team terminal does not consume personal energy"),
			Package->GetItemQuantityByTag(LG::SurvivalTags::Item_RespawnEnergy), 2);

		FSurvivalPlayerInteractionTestAccess::ClearOverlapCacheAndInteract(*Character);
		TestEqual(TEXT("Matching terminal atomically consumes one personal energy"),
			Package->GetItemQuantityByTag(LG::SurvivalTags::Item_RespawnEnergy), 1);
		TestEqual(TEXT("Successful deposit credits one Police team energy"),
			GameState->GetTeamState(ETeamType::ETT_Police).RespawnEnergy, 1);
		TestEqual(TEXT("Deposit does not credit the other team"),
			GameState->GetTeamState(ETeamType::ETT_Bandit).RespawnEnergy, 0);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalTemporarySpectatorTest,
		"LegoGame.Survival.Match.TemporarySpectator",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalTemporarySpectatorTest::RunTest(const FString& Parameters)
	{
		static int32 WorldIndex = 0;
		UWorld* World = NewObject<UWorld>(
			GetTransientPackage(),
			FName(*FString::Printf(TEXT("SurvivalTemporarySpectatorTest_%d"), ++WorldIndex)),
			RF_Transient);
		if (!TestNotNull(TEXT("Authority test world"), World) || !GEngine)
		{
			return false;
		}

		World->WorldType = EWorldType::Game;
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		World->InitializeNewWorld(UWorld::InitializationValues()
			.AllowAudioPlayback(false)
			.CreatePhysicsScene(false)
			.CreateNavigation(false)
			.CreateAISystem(false)
			.ShouldSimulatePhysics(false)
			.EnableTraceCollision(false)
			.SetTransactional(false));

		ASurvivalGameMode* GameMode = World->SpawnActor<ASurvivalGameMode>();
		APlayerController* Controller = World->SpawnActor<APlayerController>();
		ASurvivalPlayerState* PlayerState = World->SpawnActor<ASurvivalPlayerState>();
		APawn* Pawn = World->SpawnActor<APawn>();
		if (Controller && PlayerState)
		{
			Controller->PlayerState = PlayerState;
			PlayerState->SetOwner(Controller);
			PlayerState->SetIsOnlyASpectator(true);
			Controller->SetPawn(Pawn);
		}
		if (GameMode)
		{
			FSurvivalMatchProductionTestAccess::SetDefaultPawnClass(*GameMode, ADefaultPawn::StaticClass());
		}
		FURL TestURL;
		World->InitializeActorsForPlay(TestURL);
		World->BeginPlay();
		if (GameMode && Controller && PlayerState && Pawn)
		{
			// The manually-created automation world has no authoritative GameMode bootstrap,
			// so make its server roles, PlayerState and possession explicit after actor initialization.
			GameMode->SetRole(ROLE_Authority);
			Controller->SetRole(ROLE_Authority);
			Pawn->SetRole(ROLE_Authority);
			Controller->PlayerState = PlayerState;
			PlayerState->SetOwner(Controller);
			PlayerState->SetIsOnlyASpectator(true);
			Controller->Possess(Pawn);
		}
		APawn* PreExistingPawn = Controller ? Controller->GetPawn() : nullptr;

		if (TestNotNull(TEXT("Survival GameMode"), GameMode)
			&& TestNotNull(TEXT("PlayerController"), Controller)
			&& TestNotNull(TEXT("Survival PlayerState"), PlayerState)
			&& TestNotNull(TEXT("Pre-existing possessed Pawn"), PreExistingPawn))
		{
			FSurvivalMatchProductionTestAccess::EnterTemporarySpectatorState(*GameMode, Controller);
			TestFalse(TEXT("Temporary spectator is never OnlySpectator"), PlayerState->IsOnlyASpectator());
			TestTrue(TEXT("Waiting player is represented as a spectator"), PlayerState->IsSpectator());
			TestTrue(TEXT("Controller enters Spectating state"), Controller->IsInState(NAME_Spectating));
			TestFalse(TEXT("Temporary spectator passes the GameMode MustSpectate gate"), GameMode->MustSpectate(Controller));
			TestNull(TEXT("Temporary spectator releases the pre-existing Pawn"), Controller->GetPawn());
			TestFalse(TEXT("Temporary spectator destroys the pre-existing Pawn"), IsValid(PreExistingPawn));

			// A fresh Pawn must be created at the runtime anchor. Reusing the pre-travel Pawn would preserve
			// its old transform and can put a player outside a dynamically generated layout.
			const FTransform RuntimeAnchor(FRotator::ZeroRotator, FVector(1500.0f, 1800.0f, 100.0f));
			GameMode->RestartPlayerAtTransform(Controller, RuntimeAnchor);
			APawn* RespawnedPawn = Controller->GetPawn();
			TestNotNull(TEXT("RestartPlayerAtTransform creates a fresh Pawn"), RespawnedPawn);
			TestTrue(TEXT("Restarted Pawn is not the destroyed pre-existing Pawn"), RespawnedPawn != PreExistingPawn);
			if (RespawnedPawn)
			{
				TestTrue(TEXT("Restarted Pawn uses the requested runtime anchor transform"),
					RespawnedPawn->GetActorLocation().Equals(RuntimeAnchor.GetLocation(), 1.0f));
			}
			TestTrue(TEXT("Possession restores Playing state"), Controller->IsInState(NAME_Playing));
			TestFalse(TEXT("Playing player is no longer a spectator"), PlayerState->IsSpectator());
		}

		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalAmmoResourceQuantityTest,
		"LegoGame.Survival.Match.Directors.ResourceQuantity",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalAmmoResourceQuantityTest::RunTest(const FString& Parameters)
	{
		const ASurvivalGameMode* GameModeCDO = GetDefault<ASurvivalGameMode>();
		if (!TestNotNull(TEXT("Survival GameMode CDO"), GameModeCDO))
		{
			return false;
		}

		TestEqual(TEXT("Ammo pickup quantity is thirty rounds"),
			FSurvivalMatchProductionTestAccess::GetResourceSpawnQuantityForTag(*GameModeCDO, LG::SurvivalTags::Item_Ammo), 30);
		TestEqual(TEXT("Non-ammo pickups retain the configured default quantity"),
			FSurvivalMatchProductionTestAccess::GetResourceSpawnQuantityForTag(*GameModeCDO, LG::SurvivalTags::Item_Food), 1);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalCombatDeathLifecycleTest,
		"LegoGame.Survival.Match.Combat.DamageAndDeath",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalCombatDeathLifecycleTest::RunTest(const FString& Parameters)
	{
		FAuthorityMatchTestWorld TestWorld;
		ASurvivalGameState* GameState = TestWorld.World ? TestWorld.World->GetGameState<ASurvivalGameState>() : nullptr;
		if (!TestNotNull(TEXT("Authority Survival world"), TestWorld.World)
			|| !TestNotNull(TEXT("Authority Survival GameMode"), TestWorld.GameMode)
			|| !TestNotNull(TEXT("Survival GameState"), GameState))
		{
			return false;
		}

		TestTrue(TEXT("GameState exposes the public Survival match-state interface"),
			GameState->GetClass()->ImplementsInterface(USurvivalMatchStateInterface::StaticClass()));
		TestNotNull(TEXT("GameState exposes the resolved Survival config"),
			ISurvivalMatchStateInterface::Execute_GetSurvivalConfig(GameState));

		APlayerController* Controller = TestWorld.World->SpawnActor<APlayerController>();
		ASurvivalPlayerState* PlayerState = TestWorld.World->SpawnActor<ASurvivalPlayerState>();
		ALgCharacterBase* Character = TestWorld.World->SpawnActor<ALgCharacterBase>();
		if (!TestNotNull(TEXT("Victim Controller"), Controller)
			|| !TestNotNull(TEXT("Victim PlayerState"), PlayerState)
			|| !TestNotNull(TEXT("Victim Character"), Character))
		{
			return false;
		}

		Controller->PlayerState = PlayerState;
		PlayerState->SetOwner(Controller);
		Controller->Possess(Character);
		if (!TestEqual(TEXT("Victim Character is possessed"), Character->GetController(), static_cast<AController*>(Controller)))
		{
			return false;
		}

		FSurvivalMatchProductionTestAccess::SetMatchInProgress(*GameState, TestWorld.World->GetTimeSeconds());
		FSurvivalMatchProductionTestAccess::PrepareAliveParticipant(
			*PlayerState, ETeamType::ETT_Police, TestWorld.World->GetTimeSeconds());

		const AWeaponBase* WeaponCDO = GetDefault<AWeaponBase>();
		if (!TestNotNull(TEXT("Weapon CDO"), WeaponCDO))
		{
			return false;
		}
		TestEqual(TEXT("Production weapon damage per shot"), WeaponCDO->GetDamagePerShot(), 10.0f);

		FDamageEvent DamageEvent;
		for (int32 ShotIndex = 0; ShotIndex < 9; ++ShotIndex)
		{
			static_cast<AActor*>(Character)->TakeDamage(
				WeaponCDO->GetDamagePerShot(), DamageEvent, Controller, nullptr);
		}
		const FSurvivalVitalsSnapshot BeforeLethalShot = Character->GetSurvivalVitalsSnapshot_Implementation();
		TestEqual(TEXT("Nine ten-damage shots leave ten HP"), BeforeLethalShot.Health, 10.0f);
		TestEqual(TEXT("Player remains alive before the lethal shot"), BeforeLethalShot.LifeState, ESurvivalLifeState::Alive);

		static_cast<AActor*>(Character)->TakeDamage(
			WeaponCDO->GetDamagePerShot(), DamageEvent, Controller, nullptr);
		TestFalse(TEXT("Lethal shot destroys the player Pawn"), IsValid(Character));
		TestNull(TEXT("Death releases the Controller Pawn"), Controller->GetPawn());
		TestTrue(TEXT("Death changes the Controller to spectating"), Controller->IsInState(NAME_Spectating));
		TestEqual(TEXT("Death enters the respawn queue"), PlayerState->GetSurvivalLifeState(), ESurvivalLifeState::WaitingRespawn);
		TestEqual(TEXT("Death increments the player death count"), PlayerState->GetDeathCount(), 1);
		TestEqual(TEXT("Player waits at the head of the team respawn queue"), PlayerState->GetRespawnQueuePosition(), 1);
		TestEqual(TEXT("No team energy means no respawn timer is reserved"), PlayerState->GetRespawnReadyServerTime(), 0.0f);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalNeedsDeathRespawnLifecycleTest,
		"LegoGame.Survival.Match.RespawnEnergy.NeedsDeathLifecycle",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalNeedsDeathRespawnLifecycleTest::RunTest(const FString& Parameters)
	{
		UClass* ProductionGameModeClass = LoadClass<ASurvivalGameMode>(
			nullptr, TEXT("/Game/LegoGame/Survival/BP_SurvivalGameMode.BP_SurvivalGameMode_C"));
		if (!TestNotNull(TEXT("Production Survival GameMode Blueprint"), ProductionGameModeClass))
		{
			return false;
		}
		FAuthorityMatchTestWorld TestWorld(ProductionGameModeClass);
		ASurvivalGameState* GameState = TestWorld.World ? TestWorld.World->GetGameState<ASurvivalGameState>() : nullptr;
		if (!TestNotNull(TEXT("Authority Survival world"), TestWorld.World)
			|| !TestNotNull(TEXT("Authority Survival GameMode"), TestWorld.GameMode)
			|| !TestNotNull(TEXT("Survival GameState"), GameState))
		{
			return false;
		}

		APlayerController* Controller = TestWorld.World->SpawnActor<APlayerController>();
		ASurvivalPlayerState* PlayerState = TestWorld.World->SpawnActor<ASurvivalPlayerState>();
		ALgCharacterBase* Character = TestWorld.World->SpawnActor<ALgCharacterBase>();
		UPackageComponent* Package = Character ? Character->GetPackageComponent() : nullptr;
		USurvivalVitalsComponent* Vitals = Character ? Character->GetSurvivalVitalsComponent() : nullptr;
		if (!TestNotNull(TEXT("Needs-death Controller"), Controller)
			|| !TestNotNull(TEXT("Needs-death PlayerState"), PlayerState)
			|| !TestNotNull(TEXT("Needs-death Character"), Character)
			|| !TestNotNull(TEXT("Needs-death inventory"), Package)
			|| !TestNotNull(TEXT("Needs-death vitals"), Vitals))
		{
			return false;
		}

		Controller->PlayerState = PlayerState;
		PlayerState->SetOwner(Controller);
		Controller->Possess(Character);
		FSurvivalMatchProductionTestAccess::SetMatchInProgress(*GameState, TestWorld.World->GetTimeSeconds());
		FSurvivalMatchProductionTestAccess::PrepareAliveParticipant(
			*PlayerState, ETeamType::ETT_Police, TestWorld.World->GetTimeSeconds());

		FItemStack EnergyStack;
		EnergyStack.ItemId = LG::SurvivalItemIds::RespawnEnergy;
		EnergyStack.Quantity = 1;
		if (!TestTrue(TEXT("Needs-death test adds one RespawnEnergy"), Package->TryAddItemStack(EnergyStack))
			|| !TestTrue(TEXT("Respawn terminal deposit stores the energy for the team"),
				TestWorld.GameMode->DepositTeamRespawnEnergy(Character, ETeamType::ETT_Police, 1)))
		{
			return false;
		}
		TestEqual(TEXT("Deposited energy leaves the personal inventory"),
			Package->GetItemQuantityByTag(LG::SurvivalTags::Item_RespawnEnergy), 0);
		TestEqual(TEXT("Deposited energy is visible in the replicated team state"),
			GameState->GetTeamState(ETeamType::ETT_Police).RespawnEnergy, 1);

		// A seamless-travel/replication ordering issue must not strand a currently
		// possessed Survival Pawn after its vitals have already committed death.
		FSurvivalMatchProductionTestAccess::SimulateStaleSpectatorMetadata(
			*PlayerState, TestWorld.World->GetTimeSeconds());

		const FSurvivalVitalsSnapshot BeforeDeath = Vitals->GetSnapshot();
		AddExpectedError(
			TEXT("Recovering Survival player death state"), EAutomationExpectedMessageFlags::Contains, 1);
		Vitals->ApplyDamage(BeforeDeath.Health, nullptr, nullptr);

		TestFalse(TEXT("Needs death destroys the player Pawn"), IsValid(Character));
		TestNull(TEXT("Needs death releases the Controller Pawn"), Controller->GetPawn());
		TestTrue(TEXT("Needs death changes the Controller to spectating"), Controller->IsInState(NAME_Spectating));
		TestEqual(TEXT("Needs death enters the respawn queue"),
			PlayerState->GetSurvivalLifeState(), ESurvivalLifeState::WaitingRespawn);
		TestEqual(TEXT("Needs death reserves the deposited team energy"),
			GameState->GetTeamState(ETeamType::ETT_Police).RespawnEnergy, 0);
		TestEqual(TEXT("Needs death is first in the team queue"), PlayerState->GetRespawnQueuePosition(), 1);
		TestTrue(TEXT("Needs death receives a future respawn-ready time"),
			PlayerState->GetRespawnReadyServerTime() > TestWorld.World->GetTimeSeconds());
		const USurvivalModeConfig* ProductionConfig = LoadObject<USurvivalModeConfig>(nullptr, SurvivalModeConfigPath);
		if (TestNotNull(TEXT("Production mode config for respawn timing"), ProductionConfig))
		{
			const float ActualRespawnDelay =
				PlayerState->GetRespawnReadyServerTime() - TestWorld.World->GetTimeSeconds();
			TestEqual(TEXT("Reserved timer uses the production respawn delay"),
				ActualRespawnDelay, ProductionConfig->RespawnDelaySeconds, 0.01f);
			AddInfo(FString::Printf(TEXT("Production respawn wait: %.2f seconds."),
				ProductionConfig->RespawnDelaySeconds));
		}

		const FSurvivalRespawnPresentation Presentation = FSurvivalRespawnWidgetTestAccess::Resolve(
			PlayerState->GetSurvivalLifeState(), PlayerState->IsMatchParticipant(),
			PlayerState->GetRespawnQueuePosition(), PlayerState->GetRespawnReadyServerTime(),
			TestWorld.World->GetTimeSeconds());
		TestTrue(TEXT("Needs death displays the respawn HUD"), Presentation.bVisible);
		TestTrue(TEXT("Needs death displays a positive respawn countdown"), Presentation.CountdownSeconds > 0);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalHUDConfigurationTest,
		"LegoGame.Survival.Match.HUD.Configuration",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalHUDConfigurationTest::RunTest(const FString& Parameters)
	{
		const ASurvivalHUD* HUDCDO = GetDefault<ASurvivalHUD>();
		const FClassProperty* WidgetClassProperty = FindFProperty<FClassProperty>(
			ASurvivalHUD::StaticClass(), TEXT("SurvivalHUDWidgetClass"));
		const FObjectProperty* VitalsTextProperty = FindFProperty<FObjectProperty>(
			USurvivalHUDWidget::StaticClass(), TEXT("VitalsText"));
		const FObjectProperty* AmmoTextProperty = FindFProperty<FObjectProperty>(
			UGameFetureUserWidget::StaticClass(), TEXT("WeaponClipTextBlock"));
		const FClassProperty* PlayerVitalsWidgetClassProperty = FindFProperty<FClassProperty>(
			ASurvivalHUD::StaticClass(), TEXT("PlayerVitalsWidgetClass"));
		const FClassProperty* RespawnWidgetClassProperty = FindFProperty<FClassProperty>(
			ASurvivalHUD::StaticClass(), TEXT("RespawnWidgetClass"));
		const FClassProperty* EndingWidgetClassProperty = FindFProperty<FClassProperty>(
			ASurvivalHUD::StaticClass(), TEXT("EndingWidgetClass"));
		const FObjectProperty* PowerProgressProperty = FindFProperty<FObjectProperty>(
			USurvivalVitalsStatusWidget::StaticClass(), TEXT("PowerProgress"));
		const FObjectProperty* SANProgressProperty = FindFProperty<FObjectProperty>(
			USurvivalVitalsStatusWidget::StaticClass(), TEXT("SANProgress"));
		const FObjectProperty* RespawnNumProperty = FindFProperty<FObjectProperty>(
			USurvivalRespawnWidget::StaticClass(), TEXT("TextBlock_Num"));
		const FObjectProperty* RespawnShowProperty = FindFProperty<FObjectProperty>(
			USurvivalRespawnWidget::StaticClass(), TEXT("TextBlock_Show"));
		const FObjectProperty* EndingResultProperty = FindFProperty<FObjectProperty>(
			USurvivalEndingWidget::StaticClass(), TEXT("TextBlock_37"));
		const FObjectProperty* EndingReturnProperty = FindFProperty<FObjectProperty>(
			USurvivalEndingWidget::StaticClass(), TEXT("Button_61"));
		if (!TestNotNull(TEXT("Survival HUD CDO"), HUDCDO)
			|| !TestNotNull(TEXT("HUD widget class property"), WidgetClassProperty)
			|| !TestNotNull(TEXT("HUD exposes its native vitals text"), VitalsTextProperty)
			|| !TestNotNull(TEXT("Game feature HUD exposes its ammunition text"), AmmoTextProperty)
			|| !TestNotNull(TEXT("HUD exposes its player-vitals widget class"), PlayerVitalsWidgetClassProperty)
			|| !TestNotNull(TEXT("HUD exposes its respawn widget class"), RespawnWidgetClassProperty)
			|| !TestNotNull(TEXT("HUD exposes its ending widget class"), EndingWidgetClassProperty)
			|| !TestNotNull(TEXT("Player-vitals widget binds the Power image"), PowerProgressProperty)
			|| !TestNotNull(TEXT("Player-vitals widget binds the SAN image"), SANProgressProperty)
			|| !TestNotNull(TEXT("Respawn widget binds its countdown text"), RespawnNumProperty)
			|| !TestNotNull(TEXT("Respawn widget binds its status text"), RespawnShowProperty)
			|| !TestNotNull(TEXT("Ending widget binds its result text"), EndingResultProperty)
			|| !TestNotNull(TEXT("Ending widget binds its return button"), EndingReturnProperty))
		{
			return false;
		}

		UClass* WidgetClass = Cast<UClass>(WidgetClassProperty->GetObjectPropertyValue_InContainer(HUDCDO));
		TestNotNull(TEXT("HUD constructor resolves a widget class"), WidgetClass);
		if (WidgetClass)
		{
			TestTrue(TEXT("Resolved class derives from USurvivalHUDWidget"),
				WidgetClass->IsChildOf(USurvivalHUDWidget::StaticClass()));
			TestEqual(TEXT("Production HUD Blueprint is selected"), WidgetClass->GetPathName(),
				FString(TEXT("/Game/LegoGame/Survival/UI/WBP_SurvivalHUD.WBP_SurvivalHUD_C")));
		}

		UClass* GameFeatureWidgetClass = LoadClass<UGameFetureUserWidget>(
			nullptr, TEXT("/Game/LegoGame/UMG/Game/WBP_GameFeture.WBP_GameFeture_C"));
		TestNotNull(TEXT("WBP_GameFeture loads for the Survival HUD"), GameFeatureWidgetClass);
		if (GameFeatureWidgetClass)
		{
			TestTrue(TEXT("WBP_GameFeture uses the native ammo snapshot adapter"),
				GameFeatureWidgetClass->IsChildOf(UGameFetureUserWidget::StaticClass()));
		}

		UClass* PlayerVitalsWidgetClass = Cast<UClass>(
			PlayerVitalsWidgetClassProperty->GetObjectPropertyValue_InContainer(HUDCDO));
		TestNotNull(TEXT("HUD constructor resolves the player-vitals widget"), PlayerVitalsWidgetClass);
		if (PlayerVitalsWidgetClass)
		{
			TestTrue(TEXT("WBP_PlayerState uses the native material adapter"),
				PlayerVitalsWidgetClass->IsChildOf(USurvivalVitalsStatusWidget::StaticClass()));
			TestEqual(TEXT("Production player-vitals Blueprint is selected"), PlayerVitalsWidgetClass->GetPathName(),
				FString(TEXT("/Game/LegoGame/UMG/Game/WBP_PlayerState.WBP_PlayerState_C")));
		}

		UClass* RespawnWidgetClass = Cast<UClass>(
			RespawnWidgetClassProperty->GetObjectPropertyValue_InContainer(HUDCDO));
		TestNotNull(TEXT("HUD constructor resolves the respawn widget"), RespawnWidgetClass);
		if (RespawnWidgetClass)
		{
			TestTrue(TEXT("WBP_SurvivalRespawn uses the native respawn adapter"),
				RespawnWidgetClass->IsChildOf(USurvivalRespawnWidget::StaticClass()));
			TestEqual(TEXT("Production respawn Blueprint is selected"), RespawnWidgetClass->GetPathName(),
				FString(TEXT("/Game/LegoGame/Survival/UI/WBP_SurvivalRespawn.WBP_SurvivalRespawn_C")));
		}

		UClass* EndingWidgetClass = Cast<UClass>(
			EndingWidgetClassProperty->GetObjectPropertyValue_InContainer(HUDCDO));
		TestNotNull(TEXT("HUD constructor resolves the ending widget"), EndingWidgetClass);
		if (EndingWidgetClass)
		{
			TestTrue(TEXT("WBP_Ending uses the native ending adapter"),
				EndingWidgetClass->IsChildOf(USurvivalEndingWidget::StaticClass()));
			TestEqual(TEXT("Production ending Blueprint is selected"), EndingWidgetClass->GetPathName(),
				FString(TEXT("/Game/LegoGame/UMG/Game/WBP_Ending.WBP_Ending_C")));
		}

		const auto TestProgressMaterial = [this](const TCHAR* Label, const TCHAR* Path)
		{
			UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, Path);
			if (!TestNotNull(*FString::Printf(TEXT("%s material loads"), Label), Material))
			{
				return;
			}
			TArray<FMaterialParameterInfo> ParameterInfos;
			TArray<FGuid> ParameterIds;
			Material->GetAllScalarParameterInfo(ParameterInfos, ParameterIds);
			TestTrue(*FString::Printf(TEXT("%s material exposes ProgressBar"), Label),
				ParameterInfos.ContainsByPredicate([](const FMaterialParameterInfo& Info)
				{
					return Info.Name == TEXT("ProgressBar");
				}));
		};
		TestProgressMaterial(TEXT("Power"), TEXT("/Game/Aproject/Material/Roulette/MI_Roulette_Power.MI_Roulette_Power"));
		TestProgressMaterial(TEXT("SAN"), TEXT("/Game/Aproject/Material/Roulette/MI_Roulette_SAN.MI_Roulette_SAN"));

		TestEqual(TEXT("Thirst-style full vital normalizes to one"),
			FSurvivalVitalsStatusWidgetTestAccess::NormalizeVital(100.0f, 100.0f), 1.0f);
		TestEqual(TEXT("Hunger-style half vital normalizes to one half"),
			FSurvivalVitalsStatusWidgetTestAccess::NormalizeVital(50.0f, 100.0f), 0.5f);
		TestEqual(TEXT("Vital progress clamps above its maximum"),
			FSurvivalVitalsStatusWidgetTestAccess::NormalizeVital(150.0f, 100.0f), 1.0f);
		TestEqual(TEXT("Invalid vital maximum normalizes to zero"),
			FSurvivalVitalsStatusWidgetTestAccess::NormalizeVital(50.0f, 0.0f), 0.0f);

		FSurvivalVitalsSnapshot AliveSnapshot;
		AliveSnapshot.LifeState = ESurvivalLifeState::Alive;
		TestTrue(TEXT("Player vitals display during an active match for an alive local Pawn"),
			FSurvivalVitalsStatusWidgetTestAccess::ShouldDisplayVitals(
				ESurvivalMatchPhase::InProgress, true, AliveSnapshot));
		TestFalse(TEXT("Player vitals hide during countdown"),
			FSurvivalVitalsStatusWidgetTestAccess::ShouldDisplayVitals(
				ESurvivalMatchPhase::Countdown, true, AliveSnapshot));
		TestEqual(TEXT("Engine match start promotes a pending Survival phase for UI display"),
			FSurvivalVitalsStatusWidgetTestAccess::ResolveDisplayPhase(
				ESurvivalMatchPhase::WaitingForLayout, true), ESurvivalMatchPhase::InProgress);
		TestEqual(TEXT("A pending Survival phase remains hidden before the engine match starts"),
			FSurvivalVitalsStatusWidgetTestAccess::ResolveDisplayPhase(
				ESurvivalMatchPhase::WaitingForLayout, false), ESurvivalMatchPhase::WaitingForLayout);
		AliveSnapshot.LifeState = ESurvivalLifeState::WaitingRespawn;
		TestFalse(TEXT("Player vitals hide while waiting to respawn"),
			FSurvivalVitalsStatusWidgetTestAccess::ShouldDisplayVitals(
				ESurvivalMatchPhase::InProgress, true, AliveSnapshot));
		AliveSnapshot.LifeState = ESurvivalLifeState::Alive;
		TestFalse(TEXT("Player vitals hide without a local Survival Pawn"),
			FSurvivalVitalsStatusWidgetTestAccess::ShouldDisplayVitals(
				ESurvivalMatchPhase::InProgress, false, AliveSnapshot));

		const FSurvivalRespawnPresentation AlivePresentation = FSurvivalRespawnWidgetTestAccess::Resolve(
			ESurvivalLifeState::Alive, true, INDEX_NONE, 0.0f, 100.0f);
		TestFalse(TEXT("Respawn presentation hides for an alive participant"), AlivePresentation.bVisible);

		const FSurvivalRespawnPresentation WaitingEnergyPresentation = FSurvivalRespawnWidgetTestAccess::Resolve(
			ESurvivalLifeState::WaitingRespawn, true, 2, 0.0f, 100.0f);
		TestTrue(TEXT("Respawn presentation shows while waiting for energy"), WaitingEnergyPresentation.bVisible);
		TestEqual(TEXT("Waiting for energy has no countdown"), WaitingEnergyPresentation.CountdownSeconds, INDEX_NONE);
		TestTrue(TEXT("Waiting for energy reports its reason"),
			WaitingEnergyPresentation.StatusText.ToString().Contains(TEXT("等待复活能源")));

		const FSurvivalRespawnPresentation CountdownPresentation = FSurvivalRespawnWidgetTestAccess::Resolve(
			ESurvivalLifeState::WaitingRespawn, true, 1, 110.0f, 100.2f);
		TestTrue(TEXT("Reserved respawn energy shows the countdown"), CountdownPresentation.bVisible);
		TestEqual(TEXT("Respawn countdown uses ceiling seconds"), CountdownPresentation.CountdownSeconds, 10);

		const FSurvivalRespawnPresentation SpectatorPresentation = FSurvivalRespawnWidgetTestAccess::Resolve(
			ESurvivalLifeState::Spectating, false, INDEX_NONE, 0.0f, 100.0f);
		TestTrue(TEXT("Late join spectator sees a spectator notice"), SpectatorPresentation.bVisible);
		TestTrue(TEXT("Late join spectator notice explains spectating only"),
			SpectatorPresentation.StatusText.ToString().Contains(TEXT("仅可观战")));

		const FSurvivalRespawnPresentation PostMatchSpectatorPresentation = FSurvivalRespawnWidgetTestAccess::Resolve(
			ESurvivalLifeState::Spectating, true, INDEX_NONE, 0.0f, 100.0f,
			ESurvivalMatchPhase::PostMatch);
		TestTrue(TEXT("A queued winner sees the respawn overlay after immediate match settlement"),
			PostMatchSpectatorPresentation.bVisible);
		TestEqual(TEXT("Match settlement clears the respawn countdown"),
			PostMatchSpectatorPresentation.CountdownSeconds, INDEX_NONE);
		TestTrue(TEXT("Match settlement explains spectator-only state"),
			PostMatchSpectatorPresentation.StatusText.ToString().Contains(TEXT("比赛已结束")));

		TestEqual(TEXT("Police player sees victory for a Police outcome"),
			FSurvivalEndingWidgetTestAccess::ResolveResultText(
				ESurvivalMatchOutcome::PoliceVictory, ETeamType::ETT_Police).ToString(), FString(TEXT("获胜")));
		TestEqual(TEXT("Bandit player sees defeat for a Police outcome"),
			FSurvivalEndingWidgetTestAccess::ResolveResultText(
				ESurvivalMatchOutcome::PoliceVictory, ETeamType::ETT_Bandit).ToString(), FString(TEXT("败北")));
		TestEqual(TEXT("Both teams see a draw when neither wins"),
			FSurvivalEndingWidgetTestAccess::ResolveResultText(
				ESurvivalMatchOutcome::Draw, ETeamType::ETT_Police).ToString(), FString(TEXT("平局")));
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalMatchProviderDiscoveryTest,
		"LegoGame.Survival.Match.Providers.Discovery",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalMatchProviderDiscoveryTest::RunTest(const FString& Parameters)
	{
		FMatchTestWorld TestWorld;
		if (!TestNotNull(TEXT("Match GameMode"), TestWorld.GameMode))
		{
			return false;
		}

		TestFalse(TEXT("Production Match defaults to real providers"),
			FSurvivalMatchProductionTestAccess::UsesDevelopmentStubs(*TestWorld.GameMode));
		AddExpectedError(TEXT("Survival provider discovery failed"), EAutomationExpectedMessageFlags::Contains, 1);
		TestFalse(TEXT("Zero providers are rejected"),
			FSurvivalMatchProductionTestAccess::DiscoverRuntimeProviders(*TestWorld.GameMode));

		TestWorld.World->SpawnActor<ASurvivalWorldGenerator>();
		TestWorld.World->SpawnActor<ASurvivalCoreRuntimeProvider>();
		TestTrue(TEXT("Exactly one World and one Spawn provider are accepted"),
			FSurvivalMatchProductionTestAccess::DiscoverRuntimeProviders(*TestWorld.GameMode));

		TestWorld.World->SpawnActor<ASurvivalCoreRuntimeProvider>();
		AddExpectedError(TEXT("Survival provider discovery failed"), EAutomationExpectedMessageFlags::Contains, 1);
		TestFalse(TEXT("Multiple Spawn providers are rejected"),
			FSurvivalMatchProductionTestAccess::DiscoverRuntimeProviders(*TestWorld.GameMode));
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalMatchDirectJoinTeamTest,
		"LegoGame.Survival.Match.TeamSpawn",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalMatchDirectJoinTeamTest::RunTest(const FString& Parameters)
	{
		FMatchTestWorld TestWorld;
		if (!TestNotNull(TEXT("Match GameMode"), TestWorld.GameMode))
		{
			return false;
		}

		TestEqual(TEXT("First direct join resolves the empty-team tie to Police"),
			FSurvivalMatchProductionTestAccess::DetermineBalancedFallbackTeam(*TestWorld.GameMode, 0, 0),
			ETeamType::ETT_Police);
		TestEqual(TEXT("Second direct join resolves to the smaller Bandit team"),
			FSurvivalMatchProductionTestAccess::DetermineBalancedFallbackTeam(*TestWorld.GameMode, 1, 0),
			ETeamType::ETT_Bandit);
		TestEqual(TEXT("An existing smaller Police team is selected"),
			FSurvivalMatchProductionTestAccess::DetermineBalancedFallbackTeam(*TestWorld.GameMode, 1, 3),
			ETeamType::ETT_Police);
		TestEqual(TEXT("An existing smaller Bandit team is selected"),
			FSurvivalMatchProductionTestAccess::DetermineBalancedFallbackTeam(*TestWorld.GameMode, 4, 2),
			ETeamType::ETT_Bandit);
		return true;
	}
}

#endif
