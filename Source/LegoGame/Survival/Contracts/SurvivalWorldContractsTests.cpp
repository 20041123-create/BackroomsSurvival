#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "SurvivalGameplayTags.h"
#include "SurvivalInterfaces.h"
#include "UObject/Class.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSurvivalWorldContractsApiTest,
	"LegoGame.Survival.Contracts.WorldRuntimeApi",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSurvivalWorldContractsApiTest::RunTest(const FString& Parameters)
{
	const FSurvivalWorldRuntimeSnapshot Snapshot;
	TestTrue(TEXT("Default layout status is NotRequested"),
		Snapshot.LayoutStatus == ESurvivalWorldLayoutStatus::NotRequested);
	TestFalse(TEXT("Default snapshot is not successful"), Snapshot.bSucceeded);
	TestEqual(TEXT("Default unlocked phase is invalid"), Snapshot.CurrentUnlockedPhaseIndex, INDEX_NONE);
	TestEqual(TEXT("Default materialized room count is zero"), Snapshot.MaterializedRoomCount, 0);
	TestTrue(TEXT("Default failure reason is empty"), Snapshot.FailureReason.IsEmpty());

	const FSurvivalAnchorView AnchorView;
	TestFalse(TEXT("Default anchor is disabled"), AnchorView.bEnabled);
	TestTrue(TEXT("Default anchor has no team"), AnchorView.TeamType == ETeamType::ETT_None);

	const FGameplayTag RespawnBaseTag = LG::SurvivalTags::Anchor_RespawnBase.GetTag();
	TestTrue(TEXT("Respawn base native tag is valid"), RespawnBaseTag.IsValid());
	TestEqual(TEXT("Respawn base native tag name"), RespawnBaseTag.GetTagName(), FName(TEXT("Anchor.RespawnBase")));

	const UClass* InterfaceClass = USurvivalWorldRuntimeInterface::StaticClass();
	TestNotNull(TEXT("World runtime interface is reflected"), InterfaceClass);

	const auto TestInterfaceFunction = [this, InterfaceClass](const FName FunctionName, const bool bRequiresAuthority)
	{
		const UFunction* Function = InterfaceClass ? InterfaceClass->FindFunctionByName(FunctionName) : nullptr;
		TestNotNull(*FString::Printf(TEXT("%s is reflected"), *FunctionName.ToString()), Function);
		if (Function && bRequiresAuthority)
		{
			TestTrue(*FString::Printf(TEXT("%s is BlueprintAuthorityOnly"), *FunctionName.ToString()),
				Function->HasAnyFunctionFlags(FUNC_BlueprintAuthorityOnly));
		}
	};

	TestInterfaceFunction(TEXT("RequestGenerateInitialLayout"), true);
	TestInterfaceFunction(TEXT("RequestAdvanceToPhase"), true);
	TestInterfaceFunction(TEXT("GetWorldRuntimeSnapshot"), false);
	TestInterfaceFunction(TEXT("GetAnchorsByTag"), false);
	TestInterfaceFunction(TEXT("GetTeamPlayerStartTransform"), false);

	return true;
}

#endif
