#if WITH_DEV_AUTOMATION_TESTS

#include "SurvivalLayoutPlanner.h"

#include "LegoGame/Survival/Contracts/SurvivalGameplayTags.h"
#include "Misc/AutomationTest.h"

namespace LG::Survival::World
{
	namespace
	{
		FRoomConnectorDefinition MakeConnector(const TCHAR* Id, const int32 X, const int32 Y, const ERoomConnectorDirection Direction)
		{
			FRoomConnectorDefinition Connector;
			Connector.ConnectorId = FName(Id);
			Connector.Cell = FIntPoint(X, Y);
			Connector.Direction = Direction;
			return Connector;
		}

		URoomTemplateData* MakeTemplate(const TCHAR* Id, const FIntPoint Footprint, const float Weight, const FGameplayTag RoomType, const TArray<FRoomConnectorDefinition>& Connectors)
		{
			URoomTemplateData* Template = NewObject<URoomTemplateData>(GetTransientPackage());
			Template->TemplateId = FName(Id);
			Template->Footprint = Footprint;
			Template->GenerationWeight = Weight;
			Template->AllowedRoomTypes.AddTag(RoomType);
			Template->Connectors = Connectors;
			return Template;
		}

		FLayoutRequest MakeRequest(const int32 Seed)
		{
			const TArray<FRoomConnectorDefinition> FourWay = {
				MakeConnector(TEXT("N"), 0, 0, ERoomConnectorDirection::North),
				MakeConnector(TEXT("E"), 0, 0, ERoomConnectorDirection::East),
				MakeConnector(TEXT("S"), 0, 0, ERoomConnectorDirection::South),
				MakeConnector(TEXT("W"), 0, 0, ERoomConnectorDirection::West)
			};
			FLayoutRequest Request;
			Request.Seed = Seed;
			Request.MaxRoomCount = 32;
			Request.MaxGenerationAttempts = 32;
			Request.StartTemplate = MakeTemplate(TEXT("Base"), FIntPoint(1, 1), 0.0f, LG::SurvivalTags::Room_Type_Normal, FourWay);
			Request.Templates.Add(Request.StartTemplate);
			Request.Templates.Add(MakeTemplate(TEXT("Normal"), FIntPoint(1, 1), 1.0f, LG::SurvivalTags::Room_Type_Normal, FourWay));
			Request.Templates.Add(MakeTemplate(TEXT("Monster"), FIntPoint(2, 1), 1.0f, LG::SurvivalTags::Room_Type_Monster,
			{
				MakeConnector(TEXT("W"), 0, 0, ERoomConnectorDirection::West),
				MakeConnector(TEXT("E"), 1, 0, ERoomConnectorDirection::East)
			}));
			Request.Templates.Add(MakeTemplate(TEXT("HighResource"), FIntPoint(1, 2), 1.0f, LG::SurvivalTags::Room_Type_HighResource,
			{
				MakeConnector(TEXT("S"), 0, 0, ERoomConnectorDirection::South),
				MakeConnector(TEXT("N"), 0, 1, ERoomConnectorDirection::North)
			}));

			for (int32 PhaseIndex = 0; PhaseIndex < 4; ++PhaseIndex)
			{
				FSurvivalPhaseDefinition& Phase = Request.Phases.AddDefaulted_GetRef();
				Phase.PhaseIndex = PhaseIndex;
				Phase.RoomsToUnlock = 8;
				Phase.RoomTypeWeights.Add(LG::SurvivalTags::Room_Type_Normal, PhaseIndex == 1 || PhaseIndex == 2 ? 0.0f : 1.0f);
				Phase.RoomTypeWeights.Add(LG::SurvivalTags::Room_Type_Monster, PhaseIndex == 1 ? 1.0f : 0.0f);
				Phase.RoomTypeWeights.Add(LG::SurvivalTags::Room_Type_HighResource, PhaseIndex == 2 ? 1.0f : 0.0f);
			}
			return Request;
		}

		FLayoutRequest MakeLinearRequest(
			const int32 RequestedDistance,
			const int32 InitialRoomCount = 4,
			const int32 ExpansionRoomCount = 4)
		{
			FLayoutRequest Request;
			Request.Seed = 9001;
			Request.MaxRoomCount = InitialRoomCount + ExpansionRoomCount;
			Request.MaxGenerationAttempts = 8;
			Request.MinTeamStartGraphDistance = RequestedDistance;
			Request.StartTemplate = MakeTemplate(
				TEXT("LinearStart"), FIntPoint(1, 1), 0.0f, LG::SurvivalTags::Room_Type_Normal,
				{MakeConnector(TEXT("E"), 0, 0, ERoomConnectorDirection::East)});
			Request.Templates.Add(Request.StartTemplate);
			Request.Templates.Add(MakeTemplate(
				TEXT("LinearRoom"), FIntPoint(1, 1), 1.0f, LG::SurvivalTags::Room_Type_Normal,
				{
					MakeConnector(TEXT("W"), 0, 0, ERoomConnectorDirection::West),
					MakeConnector(TEXT("E"), 0, 0, ERoomConnectorDirection::East)
				}));

			FSurvivalPhaseDefinition& InitialPhase = Request.Phases.AddDefaulted_GetRef();
			InitialPhase.PhaseIndex = 0;
			InitialPhase.RoomsToUnlock = InitialRoomCount;
			InitialPhase.RoomTypeWeights.Add(LG::SurvivalTags::Room_Type_Normal, 1.0f);
			if (ExpansionRoomCount > 0)
			{
				FSurvivalPhaseDefinition& ExpansionPhase = Request.Phases.AddDefaulted_GetRef();
				ExpansionPhase.PhaseIndex = 1;
				ExpansionPhase.RoomsToUnlock = ExpansionRoomCount;
				ExpansionPhase.RoomTypeWeights.Add(LG::SurvivalTags::Room_Type_Normal, 1.0f);
			}
			return Request;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSurvivalLayoutDeterminismTest, "LegoGame.Survival.World.Layout.Determinism", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	bool FSurvivalLayoutDeterminismTest::RunTest(const FString& Parameters)
	{
		const FLayoutPlan FirstPlan = FSurvivalLayoutPlanner::Generate(MakeRequest(1337));
		const FLayoutPlan SecondPlan = FSurvivalLayoutPlanner::Generate(MakeRequest(1337));
		TestTrue(TEXT("First layout succeeds"), FirstPlan.bSucceeded);
		TestTrue(TEXT("Second layout succeeds"), SecondPlan.bSucceeded);
		TestEqual(TEXT("Same seed has the same stable hash"), FirstPlan.StableHash, SecondPlan.StableHash);
		TestEqual(TEXT("Same seed has the same room count"), FirstPlan.Rooms.Num(), SecondPlan.Rooms.Num());
		for (int32 RoomIndex = 0; RoomIndex < FirstPlan.Rooms.Num() && RoomIndex < SecondPlan.Rooms.Num(); ++RoomIndex)
		{
			TestEqual(TEXT("Room origins are deterministic"), FirstPlan.Rooms[RoomIndex].Origin, SecondPlan.Rooms[RoomIndex].Origin);
			TestEqual(TEXT("Room rotations are deterministic"), FirstPlan.Rooms[RoomIndex].RotationQuarterTurns, SecondPlan.Rooms[RoomIndex].RotationQuarterTurns);
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSurvivalLayoutTopologyTest, "LegoGame.Survival.World.Layout.Topology", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	bool FSurvivalLayoutTopologyTest::RunTest(const FString& Parameters)
	{
		const FLayoutPlan Plan = FSurvivalLayoutPlanner::Generate(MakeRequest(1337));
		FString FailureReason;
		TestTrue(TEXT("Layout succeeds"), Plan.bSucceeded);
		TestEqual(TEXT("Layout has thirty-two rooms"), Plan.Rooms.Num(), 32);
		TestEqual(TEXT("Layout tree has thirty-one connections"), Plan.Connections.Num(), 31);
		TestTrue(TEXT("No overlap and graph connectivity validate"), FSurvivalLayoutPlanner::VerifyPlan(Plan, FailureReason));
		TestTrue(TEXT("All room types appear across phase weights"),
			Plan.Rooms.ContainsByPredicate([](const FPlannedRoom& Room) { return Room.RoomType == LG::SurvivalTags::Room_Type_Normal; })
			&& Plan.Rooms.ContainsByPredicate([](const FPlannedRoom& Room) { return Room.RoomType == LG::SurvivalTags::Room_Type_Monster; })
			&& Plan.Rooms.ContainsByPredicate([](const FPlannedRoom& Room) { return Room.RoomType == LG::SurvivalTags::Room_Type_HighResource; }));
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalTeamStartDistanceTest,
		"LegoGame.Survival.World.Layout.TeamStartDistance",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalTeamStartDistanceTest::RunTest(const FString& Parameters)
	{
		const FLayoutPlan ExactPlan = FSurvivalLayoutPlanner::Generate(MakeLinearRequest(3));
		if (!TestTrue(TEXT("Exact-distance layout succeeds"), ExactPlan.bSucceeded))
		{
			return false;
		}
		TestEqual(TEXT("Exact request is recorded"), ExactPlan.RequestedTeamStartGraphDistance, 3);
		TestEqual(TEXT("Exact graph distance is applied"), ExactPlan.AppliedTeamStartGraphDistance, 3);
		TestFalse(TEXT("Exact distance does not report fallback"), ExactPlan.bUsedFarthestTeamStartFallback);
		TestEqual(TEXT("Police receives the lower endpoint"), ExactPlan.PoliceTeamStartRoom.Value, 0);
		TestEqual(TEXT("Bandit receives the higher endpoint"), ExactPlan.BanditTeamStartRoom.Value, 3);

		const FLayoutPlan ZeroPlan = FSurvivalLayoutPlanner::Generate(MakeLinearRequest(0));
		TestTrue(TEXT("Zero-distance request succeeds"), ZeroPlan.bSucceeded);
		TestEqual(TEXT("Zero selects the initial-phase graph diameter"), ZeroPlan.AppliedTeamStartGraphDistance, 3);
		TestFalse(TEXT("Zero is an explicit farthest request, not a fallback"), ZeroPlan.bUsedFarthestTeamStartFallback);
		TestEqual(TEXT("Zero request uses the lower diameter endpoint for Police"), ZeroPlan.PoliceTeamStartRoom.Value, 0);
		TestEqual(TEXT("Zero request uses the higher diameter endpoint for Bandit"), ZeroPlan.BanditTeamStartRoom.Value, 3);

		const FLayoutPlan FallbackPlan = FSurvivalLayoutPlanner::Generate(MakeLinearRequest(99));
		TestTrue(TEXT("Unreachable distance falls back successfully"), FallbackPlan.bSucceeded);
		TestTrue(TEXT("Unreachable distance reports farthest fallback"), FallbackPlan.bUsedFarthestTeamStartFallback);
		TestEqual(TEXT("Fallback is restricted to the initial-phase diameter"), FallbackPlan.AppliedTeamStartGraphDistance, 3);
		TestTrue(TEXT("Later-phase rooms are excluded from Police start selection"), FallbackPlan.PoliceTeamStartRoom.Value < 4);
		TestTrue(TEXT("Later-phase rooms are excluded from Bandit start selection"), FallbackPlan.BanditTeamStartRoom.Value < 4);

		const FLayoutPlan RepeatedPlan = FSurvivalLayoutPlanner::Generate(MakeLinearRequest(3));
		TestEqual(TEXT("Team-start selection remains deterministic"), RepeatedPlan.PoliceTeamStartRoom.Value, ExactPlan.PoliceTeamStartRoom.Value);
		TestEqual(TEXT("Team-start StableHash remains deterministic"), RepeatedPlan.StableHash, ExactPlan.StableHash);
		TestTrue(TEXT("Changing requested distance changes Survival StableHash"), ZeroPlan.StableHash != ExactPlan.StableHash);

		FLayoutRequest NegativeRequest = MakeLinearRequest(-1);
		const FLayoutPlan NegativePlan = FSurvivalLayoutPlanner::Generate(NegativeRequest);
		TestFalse(TEXT("Negative team-start distance is rejected"), NegativePlan.bSucceeded);
		TestEqual(TEXT("Negative request returns no partial layout"), NegativePlan.Rooms.Num(), 0);
		TestTrue(TEXT("Negative request has a diagnostic"), NegativePlan.FailureReason.Contains(TEXT("cannot be negative")));

		const FLayoutPlan OneInitialRoomPlan = FSurvivalLayoutPlanner::Generate(MakeLinearRequest(1, 1, 1));
		TestFalse(TEXT("A single initial room cannot host two team starts"), OneInitialRoomPlan.bSucceeded);
		TestEqual(TEXT("Insufficient initial rooms return no partial layout"), OneInitialRoomPlan.Rooms.Num(), 0);

		FLayoutPlan DisconnectedPlan = ExactPlan;
		DisconnectedPlan.Connections.RemoveAt(0);
		FString VerificationFailure;
		TestFalse(TEXT("Disconnected team-start graph fails verification"),
			FSurvivalLayoutPlanner::VerifyPlan(DisconnectedPlan, VerificationFailure));
		TestTrue(TEXT("Disconnected graph failure is diagnostic"), !VerificationFailure.IsEmpty());
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSurvivalLayoutFailureTest, "LegoGame.Survival.World.Layout.AtomicFailure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	bool FSurvivalLayoutFailureTest::RunTest(const FString& Parameters)
	{
		FLayoutRequest Request = MakeRequest(1337);
		Request.MaxRoomCount = 2;
		Request.Templates = { Request.StartTemplate };
		const FLayoutPlan Plan = FSurvivalLayoutPlanner::Generate(Request);
		TestFalse(TEXT("Impossible template set fails"), Plan.bSucceeded);
		TestEqual(TEXT("Failure returns no partial room layout"), Plan.Rooms.Num(), 0);
		return true;
	}
}

#endif
