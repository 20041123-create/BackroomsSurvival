#if WITH_DEV_AUTOMATION_TESTS

#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "LegoGame/Enemy/EnemyCharacter.h"
#include "LegoGame/Scene/SceneItemActor.h"
#include "LegoGame/Scene/LgPlayerStart.h"
#include "LegoGame/Survival/Integration/SurvivalCoreRuntimeProvider.h"
#include "LegoGame/Survival/Integration/SurvivalTeamRespawnTerminal.h"
#include "LegoGame/Survival/Integration/SurvivalWorkbenchRuntimeSpawner.h"
#include "LegoGame/Survival/SurvivalWorkbenchActor.h"
#include "LegoGame/Survival/Contracts/SurvivalDataAssets.h"
#include "LegoGame/Survival/Contracts/SurvivalGameplayTags.h"
#include "LegoGame/Survival/Match/SurvivalGameMode.h"
#include "LegoGame/Survival/World/SurvivalWorldGenerator.h"
#include "LegoGame/Survival/World/SurvivalWorldPhaseTestDriver.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

struct FSurvivalMapConfigurationTestAccess
{
	static const TSoftObjectPtr<USurvivalModeConfig>& GetModeConfig(const ASurvivalGameMode& GameMode)
	{
		return GameMode.ModeConfig;
	}

	static bool UsesDevelopmentStubs(const ASurvivalGameMode& GameMode)
	{
		return GameMode.bUseDevelopmentIntegrationStubs;
	}

	static bool AllowsDevelopmentFallbackConfig(const ASurvivalGameMode& GameMode)
	{
		return GameMode.bAllowDevelopmentFallbackConfig;
	}

	static int32 GetResourceSpawnQuantity(const ASurvivalGameMode& GameMode)
	{
		return GameMode.ResourceSpawnQuantity;
	}

	static int32 GetAmmoResourceSpawnQuantity(const ASurvivalGameMode& GameMode)
	{
		return GameMode.AmmoResourceSpawnQuantity;
	}

	static const TArray<FGameplayTag>& GetResourceItemTags(const ASurvivalGameMode& GameMode)
	{
		return GameMode.ResourceItemTags;
	}
};

namespace LG::Survival::Integration::Tests
{
	namespace
	{
		constexpr TCHAR SurvivalMapPackage[] = TEXT("/Game/LegoGame/Survival/Maps/L_SurvivalWorld");
		constexpr TCHAR SurvivalGameModeClass[] = TEXT("/Game/LegoGame/Survival/BP_SurvivalGameMode.BP_SurvivalGameMode_C");
		constexpr TCHAR SurvivalModeConfigPath[] = TEXT("/Game/LegoGame/Survival/Data/DA_SurvivalMode_Default.DA_SurvivalMode_Default");
		constexpr TCHAR EnemyClassPath[] = TEXT("/Game/LegoGame/Blueprints/Enemy/BP_Enemy.BP_Enemy_C");
		constexpr TCHAR WorkbenchClassPath[] = TEXT("/Game/LegoGame/Survival/BP_SurvivalWorkbench.BP_SurvivalWorkbench_C");

		UObject* GetObjectProperty(const UObject* Object, const TCHAR* PropertyName)
		{
			const FObjectProperty* Property = FindFProperty<FObjectProperty>(Object->GetClass(), PropertyName);
			return Property ? Property->GetObjectPropertyValue_InContainer(Object) : nullptr;
		}

		bool GetBoolProperty(const UObject* Object, const TCHAR* PropertyName, bool& OutValue)
		{
			const FBoolProperty* Property = FindFProperty<FBoolProperty>(Object->GetClass(), PropertyName);
			if (!Property)
			{
				return false;
			}
			OutValue = Property->GetPropertyValue_InContainer(Object);
			return true;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalMapConfigurationTest,
		"LegoGame.Survival.Integration.MapConfiguration",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalMapConfigurationTest::RunTest(const FString& Parameters)
	{
		UPackage* MapPackage = LoadPackage(nullptr, SurvivalMapPackage, LOAD_None);
		UWorld* MapWorld = MapPackage ? UWorld::FindWorldInPackage(MapPackage) : nullptr;
		if (!TestNotNull(TEXT("L_SurvivalWorld loads as a map package"), MapWorld))
		{
			return false;
		}

		UClass* ExpectedGameModeClass = LoadClass<AGameModeBase>(nullptr, SurvivalGameModeClass);
		TestNotNull(TEXT("BP_SurvivalGameMode generated class"), ExpectedGameModeClass);
		TestEqual(TEXT("World Settings use BP_SurvivalGameMode"),
			MapWorld->GetWorldSettings()->DefaultGameMode.Get(), ExpectedGameModeClass);

		int32 WorldProviderCount = 0;
		int32 CoreProviderCount = 0;
		int32 WorkbenchSpawnerCount = 0;
		int32 AuthoredWorkbenchCount = 0;
		int32 PhaseTestDriverCount = 0;
		int32 PoliceStartCount = 0;
		int32 BanditStartCount = 0;
		int32 GenericStartCount = 0;
		int32 PoliceTerminalCount = 0;
		int32 BanditTerminalCount = 0;
		int32 TerminalInteractionBoundsCount = 0;
		FVector PoliceStartLocation = FVector::ZeroVector;
		FVector BanditStartLocation = FVector::ZeroVector;
		ASurvivalWorldGenerator* WorldProvider = nullptr;
		ASurvivalCoreRuntimeProvider* CoreProvider = nullptr;
		ASurvivalWorkbenchRuntimeSpawner* WorkbenchSpawner = nullptr;
		for (AActor* Actor : MapWorld->PersistentLevel->Actors)
		{
			if (ASurvivalWorldGenerator* Generator = Cast<ASurvivalWorldGenerator>(Actor))
			{
				++WorldProviderCount;
				WorldProvider = Generator;
			}
			if (ASurvivalCoreRuntimeProvider* Provider = Cast<ASurvivalCoreRuntimeProvider>(Actor))
			{
				++CoreProviderCount;
				CoreProvider = Provider;
			}
			if (ASurvivalWorkbenchRuntimeSpawner* Spawner = Cast<ASurvivalWorkbenchRuntimeSpawner>(Actor))
			{
				++WorkbenchSpawnerCount;
				WorkbenchSpawner = Spawner;
			}
			if (Cast<ASurvivalWorkbenchActor>(Actor))
			{
				++AuthoredWorkbenchCount;
			}
			if (Cast<ASurvivalWorldPhaseTestDriver>(Actor))
			{
				++PhaseTestDriverCount;
			}
			if (const ALgPlayerStart* PlayerStart = Cast<ALgPlayerStart>(Actor))
			{
				if (PlayerStart->GetTeamType() == ETeamType::ETT_Police)
				{
					++PoliceStartCount;
					PoliceStartLocation = PlayerStart->GetActorLocation();
				}
				else if (PlayerStart->GetTeamType() == ETeamType::ETT_Bandit)
				{
					++BanditStartCount;
					BanditStartLocation = PlayerStart->GetActorLocation();
				}
			}
			else if (Cast<APlayerStart>(Actor))
			{
				++GenericStartCount;
			}
			if (const ASurvivalTeamRespawnTerminal* Terminal = Cast<ASurvivalTeamRespawnTerminal>(Actor))
			{
				PoliceTerminalCount += Terminal->GetTeamType() == ETeamType::ETT_Police;
				BanditTerminalCount += Terminal->GetTeamType() == ETeamType::ETT_Bandit;
				if (const USphereComponent* Bounds = Terminal->FindComponentByClass<USphereComponent>())
				{
					++TerminalInteractionBoundsCount;
					TestTrue(TEXT("Respawn terminal interaction bounds generate overlaps"), Bounds->GetGenerateOverlapEvents());
					TestTrue(TEXT("Respawn terminal interaction bounds cover the authority range"),
						Bounds->GetUnscaledSphereRadius() >= 250.0f);
				}
			}
		}

		TestEqual(TEXT("Exactly one World runtime provider"), WorldProviderCount, 1);
		TestEqual(TEXT("Exactly one Core runtime spawn provider"), CoreProviderCount, 1);
		TestEqual(TEXT("Exactly one authority workbench runtime spawner"), WorkbenchSpawnerCount, 1);
		TestEqual(TEXT("Workbenches are generated from layout anchors, not authored in the map"), AuthoredWorkbenchCount, 0);
		TestEqual(TEXT("No production phase test driver"), PhaseTestDriverCount, 0);
		TestEqual(TEXT("Exactly one Police PlayerStart is authored"), PoliceStartCount, 1);
		TestEqual(TEXT("Exactly one Bandit PlayerStart is authored"), BanditStartCount, 1);
		TestEqual(TEXT("No generic PlayerStart fallback remains"), GenericStartCount, 0);
		TestTrue(TEXT("Team starts are spatially distinct"), FVector::DistSquared(PoliceStartLocation, BanditStartLocation) > FMath::Square(100.0f));
		TestEqual(TEXT("Exactly one Police respawn terminal is authored"), PoliceTerminalCount, 1);
		TestEqual(TEXT("Exactly one Bandit respawn terminal is authored"), BanditTerminalCount, 1);
		TestEqual(TEXT("Both respawn terminals expose explicit interaction bounds"), TerminalInteractionBoundsCount, 2);

		if (WorldProvider)
		{
			bool bGenerateOnBeginPlay = true;
			TestTrue(TEXT("World provider exposes GenerateOnBeginPlay"),
				GetBoolProperty(WorldProvider, TEXT("bGenerateOnBeginPlay"), bGenerateOnBeginPlay));
			TestFalse(TEXT("Match owns the initial layout request"), bGenerateOnBeginPlay);
			UObject* WorldModeConfig = GetObjectProperty(WorldProvider, TEXT("ModeConfig"));
			TestNotNull(TEXT("World provider references the default mode config"), WorldModeConfig);
			if (WorldModeConfig)
			{
				TestEqual(TEXT("World provider references DA_SurvivalMode_Default"),
					WorldModeConfig->GetPathName(), FString(SurvivalModeConfigPath));
			}
		}

		if (CoreProvider)
		{
			TestNotNull(TEXT("Core provider uses a real SceneItemActor class"), CoreProvider->ResourceActorClass.Get());
			TestEqual(TEXT("Core provider uses BP_Enemy as its default"), CoreProvider->DefaultEnemyClass.Get(),
				LoadClass<AEnemyCharacter>(nullptr, EnemyClassPath));
			TestFalse(TEXT("Core provider default enemy archetype remains invalid"), CoreProvider->DefaultEnemyArchetypeTag.IsValid());
		}
		if (WorkbenchSpawner)
		{
			TestEqual(TEXT("Workbench spawner uses the production Blueprint class"), WorkbenchSpawner->WorkbenchActorClass.Get(),
				LoadClass<ASurvivalWorkbenchActor>(nullptr, WorkbenchClassPath));
			TestTrue(TEXT("Workbench spawner uses a positive contract observation interval"), WorkbenchSpawner->RefreshIntervalSeconds >= 0.05f);
		}

		UClass* GameModeClass = LoadClass<ASurvivalGameMode>(nullptr, SurvivalGameModeClass);
		ASurvivalGameMode* GameModeCDO = GameModeClass ? GameModeClass->GetDefaultObject<ASurvivalGameMode>() : nullptr;
		if (!TestNotNull(TEXT("Survival GameMode CDO"), GameModeCDO))
		{
			return false;
		}
		TestEqual(TEXT("GameMode references DA_SurvivalMode_Default"),
			FSurvivalMapConfigurationTestAccess::GetModeConfig(*GameModeCDO).ToSoftObjectPath().ToString(),
			FString(SurvivalModeConfigPath));
		TestFalse(TEXT("Production GameMode does not enable development stubs"),
			FSurvivalMapConfigurationTestAccess::UsesDevelopmentStubs(*GameModeCDO));
		TestFalse(TEXT("Production GameMode does not allow a fallback config"),
			FSurvivalMapConfigurationTestAccess::AllowsDevelopmentFallbackConfig(*GameModeCDO));
		TestEqual(TEXT("Production non-ammo resource quantity is one"),
			FSurvivalMapConfigurationTestAccess::GetResourceSpawnQuantity(*GameModeCDO), 1);
		TestEqual(TEXT("Production ammo resource quantity is thirty rounds"),
			FSurvivalMapConfigurationTestAccess::GetAmmoResourceSpawnQuantity(*GameModeCDO), 30);
		const TArray<FGameplayTag>& ResourceTags = FSurvivalMapConfigurationTestAccess::GetResourceItemTags(*GameModeCDO);
		TestEqual(TEXT("Production resource tag count"), ResourceTags.Num(), 6);
		const FGameplayTag ExpectedResourceTags[] = {
			LG::SurvivalTags::Item_Food,
			LG::SurvivalTags::Item_Water,
			LG::SurvivalTags::Item_Material,
			LG::SurvivalTags::Item_Ammo,
			LG::SurvivalTags::Item_RespawnEnergy,
			LG::SurvivalTags::Item_Weapon
		};
		for (const FGameplayTag& ExpectedTag : ExpectedResourceTags)
		{
			TestTrue(FString::Printf(TEXT("Production resource tags include %s"), *ExpectedTag.ToString()), ResourceTags.Contains(ExpectedTag));
		}

		USurvivalModeConfig* ModeConfig = LoadObject<USurvivalModeConfig>(nullptr, SurvivalModeConfigPath);
		if (!TestNotNull(TEXT("DA_SurvivalMode_Default loads"), ModeConfig))
		{
			return false;
		}
		TestEqual(TEXT("Production mode supports 32 rooms"), ModeConfig->MaxRoomCount, 32);
		TestEqual(TEXT("Production mode has four phases"), ModeConfig->Phases.Num(), 4);
		const int32 ExpectedRoomsToUnlock[] = { 8, 16, 24, 32 };
		for (int32 PhaseIndex = 0; PhaseIndex < UE_ARRAY_COUNT(ExpectedRoomsToUnlock) && ModeConfig->Phases.IsValidIndex(PhaseIndex); ++PhaseIndex)
		{
			TestEqual(FString::Printf(TEXT("Phase %d advances room unlock count"), PhaseIndex),
				ModeConfig->Phases[PhaseIndex].RoomsToUnlock, ExpectedRoomsToUnlock[PhaseIndex]);
		}
		return true;
	}
}

#endif
