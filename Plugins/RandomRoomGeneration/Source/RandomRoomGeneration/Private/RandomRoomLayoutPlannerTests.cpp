#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RandomRoomLayoutPlanner.h"

namespace RandomRoomGeneration
{
	namespace
	{
		FRandomRoomConnectorDefinition Connector(const TCHAR* Id, const ERandomRoomConnectorDirection Direction)
		{
			FRandomRoomConnectorDefinition Result;
			Result.ConnectorId = FName(Id);
			Result.Direction = Direction;
			return Result;
		}

		FRandomRoomTemplateDefinition Template(const TCHAR* Id, const float Weight)
		{
			FRandomRoomTemplateDefinition Result;
			Result.TemplateId = FName(Id);
			Result.GenerationWeight = Weight;
			Result.Connectors = {
				Connector(TEXT("N"), ERandomRoomConnectorDirection::North),
				Connector(TEXT("E"), ERandomRoomConnectorDirection::East),
				Connector(TEXT("S"), ERandomRoomConnectorDirection::South),
				Connector(TEXT("W"), ERandomRoomConnectorDirection::West)
			};
			return Result;
		}

		FRandomRoomGenerationRequest RequestFor(const int32 Seed, TArray<FRandomRoomTemplateDefinition>& Templates)
		{
			Templates = { Template(TEXT("Base"), 0.0f), Template(TEXT("Normal"), 1.0f) };
			FRandomRoomGenerationRequest Request;
			Request.Seed = Seed;
			Request.MaxRoomCount = 12;
			Request.MaxGenerationAttempts = 8;
			Request.StartTemplate = &Templates[0];
			for (const FRandomRoomTemplateDefinition& Item : Templates) { Request.Templates.Add(&Item); }
			FRandomRoomPhaseDefinition& Phase = Request.Phases.AddDefaulted_GetRef();
			Phase.PhaseIndex = 0;
			Phase.RoomsToUnlock = 12;
			return Request;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRandomRoomLayoutDeterminismTest, "RandomRoomGeneration.Layout.Determinism", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	bool FRandomRoomLayoutDeterminismTest::RunTest(const FString& Parameters)
	{
		TArray<FRandomRoomTemplateDefinition> FirstTemplates;
		TArray<FRandomRoomTemplateDefinition> SecondTemplates;
		const FRandomRoomLayoutPlan FirstPlan = FRandomRoomLayoutPlanner::Generate(RequestFor(1337, FirstTemplates));
		const FRandomRoomLayoutPlan SecondPlan = FRandomRoomLayoutPlanner::Generate(RequestFor(1337, SecondTemplates));
		TestTrue(TEXT("First layout succeeds"), FirstPlan.bSucceeded);
		TestTrue(TEXT("Second layout succeeds"), SecondPlan.bSucceeded);
		TestEqual(TEXT("Same seed has the same stable hash"), FirstPlan.StableHash, SecondPlan.StableHash);
		TestEqual(TEXT("Same seed has the same room count"), FirstPlan.Rooms.Num(), SecondPlan.Rooms.Num());
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRandomRoomLayoutTopologyTest, "RandomRoomGeneration.Layout.Topology", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	bool FRandomRoomLayoutTopologyTest::RunTest(const FString& Parameters)
	{
		TArray<FRandomRoomTemplateDefinition> Templates;
		const FRandomRoomLayoutPlan Plan = FRandomRoomLayoutPlanner::Generate(RequestFor(1337, Templates));
		FString FailureReason;
		TestTrue(TEXT("Layout succeeds"), Plan.bSucceeded);
		TestEqual(TEXT("Layout has twelve rooms"), Plan.Rooms.Num(), 12);
		TestTrue(TEXT("No overlap and graph connectivity validate"), FRandomRoomLayoutPlanner::VerifyPlan(Plan, FailureReason));
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRandomRoomLayoutFailureTest, "RandomRoomGeneration.Layout.AtomicFailure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	bool FRandomRoomLayoutFailureTest::RunTest(const FString& Parameters)
	{
		TArray<FRandomRoomTemplateDefinition> Templates;
		FRandomRoomGenerationRequest Request = RequestFor(1337, Templates);
		Request.MaxRoomCount = 2;
		Request.Templates = { Request.StartTemplate };
		const FRandomRoomLayoutPlan Plan = FRandomRoomLayoutPlanner::Generate(Request);
		TestFalse(TEXT("Impossible template set fails"), Plan.bSucceeded);
		TestEqual(TEXT("Failure returns no partial room layout"), Plan.Rooms.Num(), 0);
		return true;
	}
}

#endif
