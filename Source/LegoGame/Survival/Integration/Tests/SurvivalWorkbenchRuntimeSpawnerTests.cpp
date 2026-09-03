#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "LegoGame/Survival/Contracts/SurvivalGameplayTags.h"
#include "LegoGame/Survival/Integration/SurvivalWorkbenchRuntimeSpawner.h"
#include "LegoGame/Survival/SurvivalWorkbenchActor.h"
#include "Misc/AutomationTest.h"

struct FSurvivalWorkbenchRuntimeSpawnerTestAccess
{
	static void Synchronize(ASurvivalWorkbenchRuntimeSpawner& Spawner, const FSurvivalWorldRuntimeSnapshot& Snapshot,
		const TArray<FSurvivalAnchorView>& Anchors)
	{
		Spawner.SynchronizeWorkbenches(Snapshot, Anchors);
	}
};

namespace LG::Survival::Integration::Tests
{
	namespace
	{
		struct FWorkbenchSpawnerTestWorld
		{
			FWorkbenchSpawnerTestWorld()
			{
				static int32 WorldIndex = 0;
				World = NewObject<UWorld>(GetTransientPackage(),
					FName(*FString::Printf(TEXT("SurvivalWorkbenchSpawnerTest_%d"), ++WorldIndex)), RF_Transient);
				if (!World || !GEngine)
				{
					return;
				}
				World->WorldType = EWorldType::Game;
				WorldContext = &GEngine->CreateNewWorldContext(EWorldType::Game);
				WorldContext->SetCurrentWorld(World);
				World->InitializeNewWorld(UWorld::InitializationValues()
					.AllowAudioPlayback(false)
					.CreatePhysicsScene(false)
					.CreateNavigation(false)
					.CreateAISystem(false)
					.ShouldSimulatePhysics(false)
					.EnableTraceCollision(false)
					.SetTransactional(false));
				World->SpawnActor<AGameModeBase>();
			}

			~FWorkbenchSpawnerTestWorld()
			{
				if (GEngine && WorldContext)
				{
					GEngine->DestroyWorldContext(World);
				}
				if (World)
				{
					World->DestroyWorld(false);
				}
			}

			UWorld* World = nullptr;
			FWorldContext* WorldContext = nullptr;
		};

		FSurvivalAnchorView MakeWorkbenchAnchor(int32 RoomHandleValue, const FVector& Location, bool bEnabled = true)
		{
			FSurvivalAnchorView Anchor;
			Anchor.AnchorTag = LG::SurvivalTags::Anchor_Workbench;
			Anchor.RoomHandle.Value = RoomHandleValue;
			Anchor.Transform = FTransform(FRotator(0.0f, 45.0f, 0.0f), Location);
			Anchor.bEnabled = bEnabled;
			return Anchor;
		}

		int32 CountWorkbenches(UWorld& World)
		{
			int32 Count = 0;
			for (TActorIterator<ASurvivalWorkbenchActor> Iterator(&World); Iterator; ++Iterator)
			{
				++Count;
			}
			return Count;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalWorkbenchRuntimeSpawnerTest,
		"LegoGame.Survival.Integration.WorkbenchRuntimeSpawner",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalWorkbenchRuntimeSpawnerTest::RunTest(const FString& Parameters)
	{
		FWorkbenchSpawnerTestWorld TestWorld;
		ASurvivalWorkbenchRuntimeSpawner* Spawner = TestWorld.World
			? TestWorld.World->SpawnActor<ASurvivalWorkbenchRuntimeSpawner>() : nullptr;
		if (!TestNotNull(TEXT("Authority test World"), TestWorld.World)
			|| !TestNotNull(TEXT("Workbench runtime spawner"), Spawner))
		{
			return false;
		}

		// Transient automation Worlds do not establish network roles through PIE.
		// Set the server role explicitly so this exercises the same authority gate
		// used by a listen or dedicated server.
		Spawner->SetRole(ROLE_Authority);
		TestTrue(TEXT("Spawner executes only with server authority"), Spawner->HasAuthority());
		Spawner->WorkbenchActorClass = ASurvivalWorkbenchActor::StaticClass();
		FSurvivalWorldRuntimeSnapshot Snapshot;
		Snapshot.LayoutStatus = ESurvivalWorldLayoutStatus::NotRequested;
		TArray<FSurvivalAnchorView> Anchors;
		Anchors.Add(MakeWorkbenchAnchor(11, FVector(100.0f, 200.0f, 300.0f)));
		TestEqual(TEXT("No workbench before layout success"), CountWorkbenches(*TestWorld.World), 0);

		Snapshot.LayoutStatus = ESurvivalWorldLayoutStatus::Succeeded;
		Snapshot.bSucceeded = true;
		Snapshot.LayoutHash = 101;
		Snapshot.CurrentUnlockedPhaseIndex = 0;
		Snapshot.MaterializedRoomCount = 8;
		Anchors.Add(MakeWorkbenchAnchor(12, FVector(400.0f, 500.0f, 600.0f), false));
		TestFalse(TEXT("Test anchor transform is finite"), Anchors[0].Transform.ContainsNaN());
		FSurvivalWorkbenchRuntimeSpawnerTestAccess::Synchronize(*Spawner, Snapshot, Anchors);
		TestEqual(TEXT("Enabled anchor materializes immediately after layout success"), CountWorkbenches(*TestWorld.World), 1);

		ASurvivalWorkbenchActor* SpawnedWorkbench = nullptr;
		for (TActorIterator<ASurvivalWorkbenchActor> Iterator(TestWorld.World); Iterator; ++Iterator)
		{
			SpawnedWorkbench = *Iterator;
			break;
		}
		TestNotNull(TEXT("Generated workbench is a real actor"), SpawnedWorkbench);
		if (SpawnedWorkbench)
		{
			TestTrue(TEXT("Generated workbench replicates"), SpawnedWorkbench->GetIsReplicated());
			TestTrue(TEXT("Generated workbench preserves the anchor transform"),
				SpawnedWorkbench->GetActorTransform().Equals(Anchors[0].Transform, KINDA_SMALL_NUMBER));
		}

		FSurvivalWorkbenchRuntimeSpawnerTestAccess::Synchronize(*Spawner, Snapshot, Anchors);
		TestEqual(TEXT("Repeated observation does not duplicate a workbench"), CountWorkbenches(*TestWorld.World), 1);

		Snapshot.CurrentUnlockedPhaseIndex = 1;
		Snapshot.MaterializedRoomCount = 16;
		Anchors.Add(MakeWorkbenchAnchor(13, FVector(700.0f, 800.0f, 900.0f)));
		FSurvivalWorkbenchRuntimeSpawnerTestAccess::Synchronize(*Spawner, Snapshot, Anchors);
		TestEqual(TEXT("Later phase materializes only the newly enabled anchor"), CountWorkbenches(*TestWorld.World), 2);
		return true;
	}
}

#endif
