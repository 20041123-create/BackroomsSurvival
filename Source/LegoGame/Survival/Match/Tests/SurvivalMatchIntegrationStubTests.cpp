#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "LegoGame/Survival/Contracts/SurvivalDataAssets.h"
#include "LegoGame/Survival/Contracts/SurvivalGameplayTags.h"
#include "LegoGame/Survival/Match/Integration/Stubs/SurvivalMatchIntegrationStub.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSurvivalMatchIntegrationStubTest,
	"LegoGame.Survival.Match.IntegrationStub.LayoutAndDirectorLimits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSurvivalMatchIntegrationStubTest::RunTest(const FString& Parameters)
{
	USurvivalModeConfig* Config = NewObject<USurvivalModeConfig>();
	Config->RandomSeed = 20260729;
	Config->MaxRoomCount = 5;

	USurvivalMatchIntegrationStub* Stub = NewObject<USurvivalMatchIntegrationStub>();
	bool bLayoutSucceeded = false;
	Stub->RequestLayout(*Config, FSurvivalStubLayoutReady::CreateLambda([&bLayoutSucceeded](bool bSucceeded)
	{
		bLayoutSucceeded = bSucceeded;
	}));
	TestTrue(TEXT("A two-team layout is prepared"), bLayoutSucceeded);

	TMap<FGameplayTag, float> RoomWeights;
	RoomWeights.Add(LG::SurvivalTags::Room_Type_Normal, 0.7f);
	RoomWeights.Add(LG::SurvivalTags::Room_Type_Monster, 0.3f);
	TestEqual(TEXT("Initial cumulative unlock target is honored"), Stub->UnlockRoomsTo(2, RoomWeights), 2);
	TestEqual(TEXT("Later cumulative target adds rooms"), Stub->UnlockRoomsTo(4, RoomWeights), 4);
	TestEqual(TEXT("Unlock target never regresses"), Stub->UnlockRoomsTo(1, RoomWeights), 4);
	TestEqual(TEXT("Unlock target is bounded by generated rooms"), Stub->UnlockRoomsTo(99, RoomWeights), 5);

	TestTrue(TEXT("Resources can be scheduled after room unlock"), Stub->TrySpawnResource());
	TestTrue(TEXT("First enemy respects maximum alive count"), Stub->TrySpawnEnemy(2));
	TestTrue(TEXT("Second enemy respects maximum alive count"), Stub->TrySpawnEnemy(2));
	TestFalse(TEXT("Enemy director stops at maximum alive count"), Stub->TrySpawnEnemy(2));
	Stub->NotifyStubEnemyDefeated();
	TestTrue(TEXT("Enemy budget slot is available after a defeat"), Stub->TrySpawnEnemy(2));

	return true;
}

#endif
