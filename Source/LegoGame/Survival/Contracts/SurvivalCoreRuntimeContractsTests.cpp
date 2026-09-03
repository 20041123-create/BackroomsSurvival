#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "SurvivalGameplayTags.h"
#include "SurvivalInterfaces.h"
#include "UObject/Class.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSurvivalCoreRuntimeContractsApiTest,
	"LegoGame.Survival.Contracts.CoreRuntimeApi",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSurvivalCoreRuntimeContractsApiTest::RunTest(const FString& Parameters)
{
	const FSurvivalResourceSpawnRequest ResourceRequest;
	TestFalse(TEXT("Default resource request has no item tag"), ResourceRequest.ItemTag.IsValid());
	TestEqual(TEXT("Default resource quantity is safe"), ResourceRequest.Quantity, 0);
	TestTrue(TEXT("Default resource transform is identity"), ResourceRequest.SpawnTransform.Equals(FTransform::Identity));
	TestFalse(TEXT("Default resource room handle is invalid"), ResourceRequest.RoomHandle.IsValid());

	const FSurvivalEnemySpawnRequest EnemyRequest;
	TestFalse(TEXT("Default enemy tag requests the Core default"), EnemyRequest.EnemyArchetypeTag.IsValid());
	TestTrue(TEXT("Default enemy transform is identity"), EnemyRequest.SpawnTransform.Equals(FTransform::Identity));
	TestFalse(TEXT("Default enemy room handle is invalid"), EnemyRequest.RoomHandle.IsValid());
	TestEqual(TEXT("Default enemy difficulty multiplier is one"), EnemyRequest.DifficultyMultiplier, 1.0f);

	const FSurvivalRuntimeSpawnResult SpawnResult;
	TestFalse(TEXT("Default spawn result is unsuccessful"), SpawnResult.bSucceeded);
	TestEqual(TEXT("Default spawn result is uninitialized"), SpawnResult.ResultCode, ESurvivalRuntimeSpawnResultCode::Uninitialized);
	TestTrue(TEXT("Default spawn result has no Actor"), SpawnResult.SpawnedActor == nullptr);
	TestFalse(TEXT("Default spawn result has no resolved tag"), SpawnResult.ResolvedGameplayTag.IsValid());
	TestTrue(TEXT("Default spawn result has no failure text"), SpawnResult.FailureReason.IsEmpty());
	TestTrue(TEXT("Succeeded and failure codes are distinct"),
		ESurvivalRuntimeSpawnResultCode::Succeeded != ESurvivalRuntimeSpawnResultCode::InvalidRequest);
	TestTrue(TEXT("Provider unavailable and spawn failed codes are distinct"),
		ESurvivalRuntimeSpawnResultCode::ProviderUnavailable != ESurvivalRuntimeSpawnResultCode::SpawnFailed);

	const FSurvivalWeaponAmmoSnapshot WeaponSnapshot;
	TestFalse(TEXT("Default weapon snapshot has no equipped weapon"), WeaponSnapshot.bHasEquippedWeapon);
	TestEqual(TEXT("Default weapon snapshot has no loaded ammo"), WeaponSnapshot.LoadedAmmo, 0);
	TestEqual(TEXT("Default weapon snapshot has no clip capacity"), WeaponSnapshot.ClipCapacity, 0);
	TestEqual(TEXT("Default weapon snapshot has no reserve ammo"), WeaponSnapshot.ReserveAmmo, 0);

	const FGameplayTag RespawnEnergyTag = LG::SurvivalTags::Item_RespawnEnergy.GetTag();
	TestTrue(TEXT("Respawn energy native tag is valid"), RespawnEnergyTag.IsValid());
	TestEqual(TEXT("Respawn energy native tag name"), RespawnEnergyTag.GetTagName(), FName(TEXT("Item.Category.RespawnEnergy")));

	const auto TestInterfaceFunction = [this](const UClass* InterfaceClass, const FName FunctionName, const bool bRequiresAuthority)
	{
		const FString InterfaceName = InterfaceClass ? InterfaceClass->GetName() : TEXT("UnknownInterface");
		TestNotNull(*FString::Printf(TEXT("%s interface is reflected"), *InterfaceName), InterfaceClass);
		const UFunction* Function = InterfaceClass ? InterfaceClass->FindFunctionByName(FunctionName) : nullptr;
		TestNotNull(*FString::Printf(TEXT("%s is reflected"), *FunctionName.ToString()), Function);
		if (Function)
		{
			TestEqual(*FString::Printf(TEXT("%s authority metadata"), *FunctionName.ToString()),
				Function->HasAnyFunctionFlags(FUNC_BlueprintAuthorityOnly), bRequiresAuthority);
		}
	};

	const UClass* InventoryInterfaceClass = USurvivalInventoryInterface::StaticClass();
	TestInterfaceFunction(InventoryInterfaceClass, TEXT("GetItemQuantityByTag"), false);
	TestInterfaceFunction(InventoryInterfaceClass, TEXT("TryConsumeItemsByTag"), true);

	const UClass* SpawnInterfaceClass = USurvivalRuntimeSpawnInterface::StaticClass();
	TestInterfaceFunction(SpawnInterfaceClass, TEXT("TrySpawnResource"), true);
	TestInterfaceFunction(SpawnInterfaceClass, TEXT("TrySpawnEnemy"), true);

	TestInterfaceFunction(USurvivalDeathListenerInterface::StaticClass(), TEXT("HandleSurvivalDeath"), false);
	TestInterfaceFunction(USurvivalVitalsInterface::StaticClass(), TEXT("GetSurvivalVitalsSnapshot"), false);
	TestInterfaceFunction(USurvivalWeaponStateInterface::StaticClass(), TEXT("GetSurvivalWeaponAmmoSnapshot"), false);
	TestInterfaceFunction(USurvivalMatchStateInterface::StaticClass(), TEXT("GetSurvivalConfig"), false);
	TestInterfaceFunction(USurvivalMatchStateInterface::StaticClass(), TEXT("GetSurvivalMatchPhase"), false);

	return true;
}

#endif
