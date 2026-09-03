#if WITH_DEV_AUTOMATION_TESTS

#include "SurvivalWorldGenerator.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "LegoGame/Survival/Contracts/SurvivalGameplayTags.h"
#include "Misc/AutomationTest.h"
#include "SurvivalRoomRuntimeActor.h"
#include "SurvivalSemanticAnchorActor.h"

struct FSurvivalWorldRuntimeTestAccess
{
	static void DisableAutoGeneration(ASurvivalWorldGenerator& Generator)
	{
		Generator.bGenerateOnBeginPlay = false;
	}

	static void ConfigureTemplates(
		ASurvivalWorldGenerator& Generator,
		URoomTemplateData* StartTemplate,
		const TArray<URoomTemplateData*>& Templates)
	{
		Generator.StartRoomTemplate = StartTemplate;
		Generator.RoomTemplates.Reset(Templates.Num());
		for (URoomTemplateData* Template : Templates)
		{
			Generator.RoomTemplates.Add(Template);
		}
	}

	static void SetFallbackRoomClass(
		ASurvivalWorldGenerator& Generator,
		const TSubclassOf<ASurvivalRoomRuntimeActor> RoomClass)
	{
		Generator.FallbackRoomActorClass = RoomClass;
	}

	static int32 GetSpawnedRoomCount(const ASurvivalWorldGenerator& Generator)
	{
		return Generator.SpawnedRooms.Num();
	}

	static const LG::Survival::World::FLayoutPlan& GetLayoutPlan(const ASurvivalWorldGenerator& Generator)
	{
		return Generator.LayoutPlan;
	}
};

namespace LG::Survival::World::RuntimeTests
{
	namespace
	{
		struct FRuntimeTestWorld
		{
			FRuntimeTestWorld()
			{
				static int32 WorldIndex = 0;
				const FName WorldName(*FString::Printf(TEXT("SurvivalWorldRuntimeTest_%d"), ++WorldIndex));
				World = UWorld::CreateWorld(EWorldType::Game, false, WorldName);
				if (!World || !GEngine)
				{
					return;
				}

				FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
				WorldContext.SetCurrentWorld(World);

				FActorSpawnParameters SpawnParameters;
				SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				Generator = World->SpawnActor<ASurvivalWorldGenerator>(
					ASurvivalWorldGenerator::StaticClass(),
					FTransform::Identity,
					SpawnParameters);
				if (Generator)
				{
					FSurvivalWorldRuntimeTestAccess::DisableAutoGeneration(*Generator);
				}

				FURL URL;
				World->InitializeActorsForPlay(URL);
				World->BeginPlay();
			}

			~FRuntimeTestWorld()
			{
				if (!World)
				{
					return;
				}
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
				World->DestroyWorld(false);
			}

			UWorld* World = nullptr;
			ASurvivalWorldGenerator* Generator = nullptr;
		};

		FRoomConnectorDefinition MakeConnector(
			const TCHAR* ConnectorId,
			const ERoomConnectorDirection Direction)
		{
			FRoomConnectorDefinition Connector;
			Connector.ConnectorId = FName(ConnectorId);
			Connector.Cell = FIntPoint::ZeroValue;
			Connector.Direction = Direction;
			return Connector;
		}

		URoomTemplateData* MakeFourWayTemplate(
			const TCHAR* TemplateId,
			const float GenerationWeight)
		{
			URoomTemplateData* Template = NewObject<URoomTemplateData>(GetTransientPackage());
			Template->TemplateId = FName(TemplateId);
			Template->Footprint = FIntPoint(1, 1);
			Template->GenerationWeight = GenerationWeight;
			Template->AllowedRoomTypes.AddTag(LG::SurvivalTags::Room_Type_Normal);
			Template->Connectors = {
				MakeConnector(TEXT("North"), ERoomConnectorDirection::North),
				MakeConnector(TEXT("East"), ERoomConnectorDirection::East),
				MakeConnector(TEXT("South"), ERoomConnectorDirection::South),
				MakeConnector(TEXT("West"), ERoomConnectorDirection::West)
			};
			return Template;
		}

		USurvivalModeConfig* MakeConfig()
		{
			USurvivalModeConfig* Config = NewObject<USurvivalModeConfig>(GetTransientPackage());
			Config->RandomSeed = 24680;
			Config->MaxRoomCount = 4;
			Config->MaxGenerationAttempts = 32;
			Config->MinTeamStartGraphDistance = 1;

			FSurvivalPhaseDefinition& InitialPhase = Config->Phases.AddDefaulted_GetRef();
			InitialPhase.PhaseIndex = 0;
			InitialPhase.RoomsToUnlock = 2;
			InitialPhase.RoomTypeWeights.Add(LG::SurvivalTags::Room_Type_Normal, 1.0f);

			FSurvivalPhaseDefinition& ExpansionPhase = Config->Phases.AddDefaulted_GetRef();
			ExpansionPhase.PhaseIndex = 1;
			ExpansionPhase.RoomsToUnlock = 2;
			ExpansionPhase.RoomTypeWeights.Add(LG::SurvivalTags::Room_Type_Normal, 1.0f);
			return Config;
		}

		bool ConfigureSuccessfulGeneration(
			FAutomationTestBase& Test,
			FRuntimeTestWorld& TestWorld,
			USurvivalModeConfig*& OutConfig)
		{
			if (!Test.TestNotNull(TEXT("Runtime test world"), TestWorld.World)
				|| !Test.TestNotNull(TEXT("Runtime provider"), TestWorld.Generator))
			{
				return false;
			}

			OutConfig = MakeConfig();
			URoomTemplateData* StartTemplate = MakeFourWayTemplate(TEXT("RuntimeTest.Start"), 0.0f);
			URoomTemplateData* NormalTemplate = MakeFourWayTemplate(TEXT("RuntimeTest.Normal"), 1.0f);
			FSurvivalWorldRuntimeTestAccess::ConfigureTemplates(
				*TestWorld.Generator,
				StartTemplate,
				{StartTemplate, NormalTemplate});
			return true;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalWorldRuntimeDefaultTest,
		"LegoGame.Survival.World.Runtime.DefaultAndAuthority",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalWorldRuntimeDefaultTest::RunTest(const FString& Parameters)
	{
		FRuntimeTestWorld TestWorld;
		if (!TestNotNull(TEXT("Runtime provider"), TestWorld.Generator))
		{
			return false;
		}

		const FSurvivalWorldRuntimeSnapshot DefaultSnapshot =
			ISurvivalWorldRuntimeInterface::Execute_GetWorldRuntimeSnapshot(TestWorld.Generator);
		TestEqual(TEXT("Default status is NotRequested"), DefaultSnapshot.LayoutStatus, ESurvivalWorldLayoutStatus::NotRequested);
		TestFalse(TEXT("Default snapshot is not successful"), DefaultSnapshot.bSucceeded);
		TestEqual(TEXT("Default phase is unset"), DefaultSnapshot.CurrentUnlockedPhaseIndex, INDEX_NONE);
		TestEqual(TEXT("Default materialized room count is zero"), DefaultSnapshot.MaterializedRoomCount, 0);

		TestFalse(
			TEXT("Null config is rejected"),
			ISurvivalWorldRuntimeInterface::Execute_RequestGenerateInitialLayout(TestWorld.Generator, nullptr));
		TestEqual(
			TEXT("Null config does not consume the one-shot request"),
			ISurvivalWorldRuntimeInterface::Execute_GetWorldRuntimeSnapshot(TestWorld.Generator).LayoutStatus,
			ESurvivalWorldLayoutStatus::NotRequested);

		USurvivalModeConfig* Config = MakeConfig();
		TestWorld.Generator->SetRole(ROLE_SimulatedProxy);
		TestFalse(
			TEXT("Non-authority generation request is rejected"),
			ISurvivalWorldRuntimeInterface::Execute_RequestGenerateInitialLayout(TestWorld.Generator, Config));
		TestFalse(
			TEXT("Non-authority phase request is rejected"),
			ISurvivalWorldRuntimeInterface::Execute_RequestAdvanceToPhase(TestWorld.Generator, 0));
		TestWorld.Generator->SetRole(ROLE_Authority);

		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalWorldRuntimeFailureTest,
		"LegoGame.Survival.World.Runtime.FailureSnapshot",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalWorldRuntimeFailureTest::RunTest(const FString& Parameters)
	{
		FRuntimeTestWorld TestWorld;
		if (!TestNotNull(TEXT("Runtime provider"), TestWorld.Generator))
		{
			return false;
		}

		USurvivalModeConfig* Config = MakeConfig();
		AddExpectedError(
			TEXT("Survival world generation needs a mode config and a start room template."),
			EAutomationExpectedErrorFlags::Contains,
			1);
		TestTrue(
			TEXT("Valid one-shot request is accepted even when generation later fails"),
			ISurvivalWorldRuntimeInterface::Execute_RequestGenerateInitialLayout(TestWorld.Generator, Config));

		const FSurvivalWorldRuntimeSnapshot Snapshot =
			ISurvivalWorldRuntimeInterface::Execute_GetWorldRuntimeSnapshot(TestWorld.Generator);
		TestEqual(TEXT("Missing start template records Failed"), Snapshot.LayoutStatus, ESurvivalWorldLayoutStatus::Failed);
		TestFalse(TEXT("Failed snapshot is not successful"), Snapshot.bSucceeded);
		TestTrue(TEXT("Failed snapshot has a diagnostic"), !Snapshot.FailureReason.IsEmpty());
		TestFalse(
			TEXT("Failed provider rejects duplicate generation"),
			ISurvivalWorldRuntimeInterface::Execute_RequestGenerateInitialLayout(TestWorld.Generator, Config));
		TestFalse(
			TEXT("Failed provider rejects phase advancement"),
			ISurvivalWorldRuntimeInterface::Execute_RequestAdvanceToPhase(TestWorld.Generator, 1));
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalWorldRuntimePhaseTest,
		"LegoGame.Survival.World.Runtime.GenerationAndPhases",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalWorldRuntimePhaseTest::RunTest(const FString& Parameters)
	{
		FRuntimeTestWorld TestWorld;
		USurvivalModeConfig* Config = nullptr;
		if (!ConfigureSuccessfulGeneration(*this, TestWorld, Config))
		{
			return false;
		}

		TestTrue(
			TEXT("Initial layout request is accepted"),
			ISurvivalWorldRuntimeInterface::Execute_RequestGenerateInitialLayout(TestWorld.Generator, Config));
		const FSurvivalWorldRuntimeSnapshot InitialSnapshot =
			ISurvivalWorldRuntimeInterface::Execute_GetWorldRuntimeSnapshot(TestWorld.Generator);
		TestEqual(TEXT("Layout generation succeeds"), InitialSnapshot.LayoutStatus, ESurvivalWorldLayoutStatus::Succeeded);
		TestTrue(TEXT("Successful status sets bSucceeded"), InitialSnapshot.bSucceeded);
		TestEqual(TEXT("Applied seed is reported"), InitialSnapshot.AppliedSeed, Config->RandomSeed);
		TestTrue(TEXT("Layout hash is populated"), InitialSnapshot.LayoutHash != 0);
		TestEqual(TEXT("Initial phase is zero"), InitialSnapshot.CurrentUnlockedPhaseIndex, 0);
		TestEqual(TEXT("Only the two initial-phase rooms are materialized"), InitialSnapshot.MaterializedRoomCount, 2);
		TestFalse(
			TEXT("Successful provider rejects duplicate generation"),
			ISurvivalWorldRuntimeInterface::Execute_RequestGenerateInitialLayout(TestWorld.Generator, Config));
		TestFalse(
			TEXT("Current phase is rejected"),
			ISurvivalWorldRuntimeInterface::Execute_RequestAdvanceToPhase(TestWorld.Generator, 0));
		TestFalse(
			TEXT("Lower phase is rejected"),
			ISurvivalWorldRuntimeInterface::Execute_RequestAdvanceToPhase(TestWorld.Generator, -1));
		TestFalse(
			TEXT("Undefined phase is rejected"),
			ISurvivalWorldRuntimeInterface::Execute_RequestAdvanceToPhase(TestWorld.Generator, 99));

		FSurvivalWorldRuntimeTestAccess::SetFallbackRoomClass(*TestWorld.Generator, nullptr);
		AddExpectedError(
			TEXT("Survival world could not spawn planned room"),
			EAutomationExpectedErrorFlags::Contains,
			1);
		TestFalse(
			TEXT("Room materialization failure rejects phase advancement"),
			ISurvivalWorldRuntimeInterface::Execute_RequestAdvanceToPhase(TestWorld.Generator, 1));
		const FSurvivalWorldRuntimeSnapshot FailedAdvanceSnapshot =
			ISurvivalWorldRuntimeInterface::Execute_GetWorldRuntimeSnapshot(TestWorld.Generator);
		TestEqual(
			TEXT("Failed advancement preserves current phase"),
			FailedAdvanceSnapshot.CurrentUnlockedPhaseIndex,
			InitialSnapshot.CurrentUnlockedPhaseIndex);
		TestEqual(
			TEXT("Failed advancement preserves reported room count"),
			FailedAdvanceSnapshot.MaterializedRoomCount,
			InitialSnapshot.MaterializedRoomCount);
		TestEqual(
			TEXT("Failed advancement preserves actual spawned room count"),
			FSurvivalWorldRuntimeTestAccess::GetSpawnedRoomCount(*TestWorld.Generator),
			InitialSnapshot.MaterializedRoomCount);

		FSurvivalWorldRuntimeTestAccess::SetFallbackRoomClass(
			*TestWorld.Generator,
			ASurvivalRoomRuntimeActor::StaticClass());
		TestTrue(
			TEXT("Defined higher phase advances after materialization is available"),
			ISurvivalWorldRuntimeInterface::Execute_RequestAdvanceToPhase(TestWorld.Generator, 1));
		const FSurvivalWorldRuntimeSnapshot ExpandedSnapshot =
			ISurvivalWorldRuntimeInterface::Execute_GetWorldRuntimeSnapshot(TestWorld.Generator);
		TestEqual(TEXT("Expanded phase is reported"), ExpandedSnapshot.CurrentUnlockedPhaseIndex, 1);
		TestEqual(TEXT("All planned rooms are materialized"), ExpandedSnapshot.MaterializedRoomCount, 4);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalWorldRoomGeometryNetworkingTest,
		"LegoGame.Survival.World.Runtime.RoomGeometryNetworking",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalWorldRoomGeometryNetworkingTest::RunTest(const FString& Parameters)
	{
		FRuntimeTestWorld TestWorld;
		USurvivalModeConfig* Config = nullptr;
		if (!ConfigureSuccessfulGeneration(*this, TestWorld, Config)
			|| !TestTrue(
				TEXT("Initial layout request is accepted"),
				ISurvivalWorldRuntimeInterface::Execute_RequestGenerateInitialLayout(TestWorld.Generator, Config)))
		{
			return false;
		}

		ASurvivalRoomRuntimeActor* Room = nullptr;
		for (TActorIterator<ASurvivalRoomRuntimeActor> Iterator(TestWorld.World); Iterator; ++Iterator)
		{
			if (Iterator->GetOwner() == TestWorld.Generator)
			{
				Room = *Iterator;
				break;
			}
		}
		if (!TestNotNull(TEXT("Generated room"), Room))
		{
			return false;
		}

		TInlineComponentArray<UInstancedStaticMeshComponent*> GeometryBatches(Room);
		TestEqual(TEXT("Room owns one fixed geometry batch per mesh category"), GeometryBatches.Num(), 8);
		UBoxComponent* NavigationFloor = Room->FindComponentByClass<UBoxComponent>();
		if (TestNotNull(TEXT("Room owns a stable simple navigation floor"), NavigationFloor))
		{
			TestTrue(TEXT("Navigation floor is a stable default subobject"), NavigationFloor->IsDefaultSubobject());
			TestEqual(TEXT("Navigation floor provides query and physics collision"), NavigationFloor->GetCollisionEnabled(), ECollisionEnabled::QueryAndPhysics);
			TestTrue(TEXT("Navigation floor contributes upward-facing Recast geometry"), NavigationFloor->CanEverAffectNavigation());
			TestTrue(TEXT("Navigation floor covers a non-zero room footprint"),
				NavigationFloor->GetUnscaledBoxExtent().X > 0.0f && NavigationFloor->GetUnscaledBoxExtent().Y > 0.0f);
			TestTrue(TEXT("Navigation floor top remains aligned with generated visual tiles"),
				FMath::IsNearlyZero(NavigationFloor->GetRelativeLocation().Z + NavigationFloor->GetUnscaledBoxExtent().Z));
		}
		for (UInstancedStaticMeshComponent* Component : GeometryBatches)
		{
			if (!TestNotNull(TEXT("Geometry batch"), Component))
			{
				continue;
			}
			TestTrue(TEXT("Geometry batch is a stable default subobject"), Component->IsDefaultSubobject());
			TestTrue(TEXT("Geometry batch supports NetGUID references"), Component->IsSupportedForNetworking());
			TestFalse(TEXT("Geometry batch does not use an auto-generated runtime name"), Component->GetName().StartsWith(TEXT("InstancedStaticMeshComponent_")));
			if (Component->GetFName() == TEXT("FloorGeometry"))
			{
				TestEqual(TEXT("Visual floor delegates collision to the simple navigation floor"),
					Component->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
				TestFalse(TEXT("Visual floor no longer exports unreliable zero-thickness navigation geometry"),
					Component->CanEverAffectNavigation());
			}
			else if (Component->GetFName() == TEXT("CeilingBackingGeometry"))
			{
				TestEqual(TEXT("Ceiling backing enables camera query collision"),
					Component->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);
				TestEqual(TEXT("Ceiling backing blocks the spring-arm camera channel"),
					Component->GetCollisionResponseToChannel(ECC_Camera), ECR_Block);
				TestEqual(TEXT("Ceiling backing does not block pawn movement"),
					Component->GetCollisionResponseToChannel(ECC_Pawn), ECR_Ignore);
				TestEqual(TEXT("Ceiling backing does not alter visibility or weapon traces"),
					Component->GetCollisionResponseToChannel(ECC_Visibility), ECR_Ignore);
				TestFalse(TEXT("Camera blocker does not contribute navigation geometry"),
					Component->CanEverAffectNavigation());
				TestTrue(TEXT("Camera blocker covers the generated room"), Component->GetInstanceCount() > 0);

				FTransform BackingTransform;
				if (TestTrue(TEXT("Camera blocker exposes its world transform"),
					Component->GetInstanceTransform(0, BackingTransform, true)))
				{
					const FVector TraceCenter = BackingTransform.GetLocation();
					FHitResult CameraHit;
					const bool bCameraBlocked = TestWorld.World->LineTraceSingleByChannel(
						CameraHit,
						TraceCenter - FVector(0.0f, 0.0f, 100.0f),
						TraceCenter + FVector(0.0f, 0.0f, 100.0f),
						ECC_Camera);
					TestTrue(TEXT("Camera-channel trace is physically blocked by the roof"), bCameraBlocked);
					TestEqual(TEXT("Camera-channel trace hits the stable ceiling backing"),
						CameraHit.GetComponent(), static_cast<UPrimitiveComponent*>(Component));
				}
			}
		}

		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalWorldRuntimeAnchorTest,
		"LegoGame.Survival.World.Runtime.AnchorsAndTeamStarts",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalWorldRuntimeAnchorTest::RunTest(const FString& Parameters)
	{
		FRuntimeTestWorld TestWorld;
		USurvivalModeConfig* Config = nullptr;
		if (!ConfigureSuccessfulGeneration(*this, TestWorld, Config))
		{
			return false;
		}
		if (!TestTrue(
			TEXT("Initial layout request is accepted"),
			ISurvivalWorldRuntimeInterface::Execute_RequestGenerateInitialLayout(TestWorld.Generator, Config)))
		{
			return false;
		}

		TArray<FSurvivalAnchorView> Anchors;
		Anchors.AddDefaulted();
		ISurvivalWorldRuntimeInterface::Execute_GetAnchorsByTag(TestWorld.Generator, FGameplayTag(), Anchors);
		TestEqual(TEXT("Invalid tag clears previous output"), Anchors.Num(), 0);

		ISurvivalWorldRuntimeInterface::Execute_GetAnchorsByTag(
			TestWorld.Generator,
			LG::SurvivalTags::Anchor_PlayerStart,
			Anchors);
		TestEqual(TEXT("Initial layout exposes two player starts"), Anchors.Num(), 2);
		if (Anchors.Num() == 2)
		{
			TestTrue(TEXT("Player starts carry a valid room handle"), Anchors[0].RoomHandle.IsValid() && Anchors[1].RoomHandle.IsValid());
			TestTrue(TEXT("Player starts belong to different selected rooms"), Anchors[0].RoomHandle.Value != Anchors[1].RoomHandle.Value);
			TestTrue(TEXT("Player starts are enabled"), Anchors[0].bEnabled && Anchors[1].bEnabled);
			TestTrue(TEXT("Player start transforms are distinct"), !Anchors[0].Transform.Equals(Anchors[1].Transform));
			TestEqual(TEXT("Stable order puts Police first"), Anchors[0].TeamType, ETeamType::ETT_Police);
			TestEqual(TEXT("Stable order puts Bandit second"), Anchors[1].TeamType, ETeamType::ETT_Bandit);
		}
		const FLayoutPlan& LayoutPlan = FSurvivalWorldRuntimeTestAccess::GetLayoutPlan(*TestWorld.Generator);
		TestEqual(TEXT("Runtime request forwards the configured team-start distance"),
			LayoutPlan.RequestedTeamStartGraphDistance, Config->MinTeamStartGraphDistance);
		TestEqual(TEXT("Runtime layout applies the exact initial-room distance"),
			LayoutPlan.AppliedTeamStartGraphDistance, 1);
		TestFalse(TEXT("Runtime exact team-start distance does not fall back"),
			LayoutPlan.bUsedFarthestTeamStartFallback);
		for (TActorIterator<ASurvivalSemanticAnchorActor> Iterator(TestWorld.World); Iterator; ++Iterator)
		{
			if (Iterator->GetOwner() == TestWorld.Generator)
			{
				TestTrue(TEXT("Runtime-projected anchor transforms replicate"), Iterator->IsReplicatingMovement());
			}
		}

		FTransform PoliceTransform = FTransform::Identity;
		FTransform BanditTransform = FTransform::Identity;
		TestTrue(
			TEXT("Police gets its enabled player start"),
			ISurvivalWorldRuntimeInterface::Execute_GetTeamPlayerStartTransform(
				TestWorld.Generator,
				ETeamType::ETT_Police,
				PoliceTransform));
		TestTrue(
			TEXT("Bandit gets its enabled player start"),
			ISurvivalWorldRuntimeInterface::Execute_GetTeamPlayerStartTransform(
				TestWorld.Generator,
				ETeamType::ETT_Bandit,
				BanditTransform));
		TestTrue(TEXT("Police and Bandit starts are not mixed"), !PoliceTransform.Equals(BanditTransform));

		for (TActorIterator<ASurvivalSemanticAnchorActor> Iterator(TestWorld.World); Iterator; ++Iterator)
		{
			ASurvivalSemanticAnchorActor* Anchor = *Iterator;
			if (Anchor
				&& Anchor->GetOwner() == TestWorld.Generator
				&& Anchor->GetAnchorTag() == LG::SurvivalTags::Anchor_PlayerStart
				&& Anchor->GetTeamType() == ETeamType::ETT_Police)
			{
				Anchor->SetAnchorEnabled(false);
				break;
			}
		}

		const FTransform SentinelTransform(FRotator(11.0f, 22.0f, 33.0f), FVector(101.0f, 202.0f, 303.0f));
		FTransform MissingTransform = SentinelTransform;
		TestFalse(
			TEXT("Disabled Police start is not returned"),
			ISurvivalWorldRuntimeInterface::Execute_GetTeamPlayerStartTransform(
				TestWorld.Generator,
				ETeamType::ETT_Police,
				MissingTransform));
		TestTrue(TEXT("Missing start leaves output unchanged"), MissingTransform.Equals(SentinelTransform));

		ISurvivalWorldRuntimeInterface::Execute_GetAnchorsByTag(
			TestWorld.Generator,
			LG::SurvivalTags::Anchor_PlayerStart,
			Anchors);
		TestEqual(TEXT("Disabled anchors remain visible in anchor queries"), Anchors.Num(), 2);
		TestTrue(
			TEXT("Public anchor view reports disabled state"),
			Anchors.ContainsByPredicate([](const FSurvivalAnchorView& Anchor)
			{
				return Anchor.TeamType == ETeamType::ETT_Police && !Anchor.bEnabled;
			}));

		ISurvivalWorldRuntimeInterface::Execute_GetAnchorsByTag(
			TestWorld.Generator,
			LG::SurvivalTags::Anchor_RespawnBase,
			Anchors);
		TestEqual(TEXT("Police and Bandit have distinct RespawnBase anchors"), Anchors.Num(), 2);
		TestTrue(
			TEXT("RespawnBase anchors are team-specific"),
			Anchors.Num() == 2
				&& Anchors[0].TeamType == ETeamType::ETT_Police
				&& Anchors[1].TeamType == ETeamType::ETT_Bandit);
		if (Anchors.Num() == 2)
		{
			TestEqual(TEXT("Police RespawnBase follows the Police start room"),
				Anchors[0].RoomHandle.Value, LayoutPlan.PoliceTeamStartRoom.Value);
			TestEqual(TEXT("Bandit RespawnBase follows the Bandit start room"),
				Anchors[1].RoomHandle.Value, LayoutPlan.BanditTeamStartRoom.Value);
		}

		ISurvivalWorldRuntimeInterface::Execute_GetAnchorsByTag(
			TestWorld.Generator,
			LG::SurvivalTags::Anchor_Resource,
			Anchors);
		TestEqual(TEXT("Selected normal start rooms retain their resource anchors"), Anchors.Num(), 2);

		ISurvivalWorldRuntimeInterface::Execute_GetAnchorsByTag(
			TestWorld.Generator,
			LG::SurvivalTags::Anchor_TeamTerminal,
			Anchors);
		TestEqual(TEXT("TeamTerminal is not used as a respawn fallback"), Anchors.Num(), 0);
		return true;
	}
}

#endif
