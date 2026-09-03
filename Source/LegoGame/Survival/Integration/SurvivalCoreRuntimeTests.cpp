#if WITH_DEV_AUTOMATION_TESTS

#include "SurvivalCoreRuntimeProvider.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "LegoGame/Character/LgCharacterBase.h"
#include "LegoGame/Components/PackageComponent.h"
#include "LegoGame/Enemy/EnemyCharacter.h"
#include "LegoGame/Data/PackageItemData.h"
#include "LegoGame/GamePlay/GameMenu/Game/PackageListViewWidget.h"
#include "LegoGame/Scene/SceneItemActor.h"
#include "LegoGame/Subsystem/PropsSubsystem.h"
#include "LegoGame/Survival/Contracts/SurvivalInterfaces.h"
#include "LegoGame/Survival/Contracts/SurvivalGameplayTags.h"
#include "LegoGame/Survival/SurvivalItemIds.h"
#include "LegoGame/Survival/SurvivalVitalsComponent.h"
#include "LegoGame/Weapon/WeaponBase.h"
#include "Misc/AutomationTest.h"

struct FPackageListViewWidgetTestAccess
{
	static void ConfigureSurvivalStack(UPackageItemData& ItemData, int32 ItemId, int32 SlotId)
	{
		ItemData.ID = ItemId;
		ItemData.SurvivalSlotId = SlotId;
		ItemData.Quantity = 1;
		ItemData.bIsSurvivalStack = true;
	}

	static void ConfigureNonSurvivalItem(UPackageItemData& ItemData, int32 ItemId)
	{
		ItemData.ID = ItemId;
		ItemData.SurvivalSlotId = INDEX_NONE;
		ItemData.Quantity = 1;
		ItemData.bIsSurvivalStack = false;
	}

	static bool RequestUse(UPackageListViewWidget& Widget, UPackageItemData& ItemData, UPackageComponent* Package)
	{
		return Widget.RequestUseSurvivalConsumable(&ItemData, Package);
	}
};

namespace LG::Survival::Core::RuntimeTests
{
	namespace
	{
		struct FRuntimeTestWorld
		{
			FRuntimeTestWorld()
			{
				static int32 WorldIndex = 0;
				const FName WorldName(*FString::Printf(TEXT("SurvivalCoreRuntimeTest_%d"), ++WorldIndex));
				World = UWorld::CreateWorld(EWorldType::Game, false, WorldName);
				if (!World || !GEngine)
				{
					return;
				}

				WorldContext = &GEngine->CreateNewWorldContext(EWorldType::Game);
				WorldContext->SetCurrentWorld(World);
				GameInstance = NewObject<UGameInstance>(GetTransientPackage());
				GameInstance->Init();
				World->SetGameInstance(GameInstance);

				FActorSpawnParameters SpawnParameters;
				SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				Provider = World->SpawnActor<ASurvivalCoreRuntimeProvider>(
					ASurvivalCoreRuntimeProvider::StaticClass(), FTransform::Identity, SpawnParameters);

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

			UPropsSubsystem* GetProps() const
			{
				return GameInstance ? GameInstance->GetSubsystem<UPropsSubsystem>() : nullptr;
			}

			UWorld* World = nullptr;
			FWorldContext* WorldContext = nullptr;
			UGameInstance* GameInstance = nullptr;
			ASurvivalCoreRuntimeProvider* Provider = nullptr;
		};

		bool FindTaggedItem(
			FAutomationTestBase& Test,
			UPropsSubsystem* Props,
			FGameplayTag ItemTag,
			int32& OutItemId)
		{
			OutItemId = INDEX_NONE;
			if (!Test.TestNotNull(TEXT("Props subsystem"), Props))
			{
				return false;
			}

			TArray<int32> ItemIds;
			Props->GetSurvivalItemIdsByTag(ItemTag, ItemIds);
			if (!Test.TestTrue(TEXT("Configured props data contains the requested formal Survival tag"), !ItemIds.IsEmpty()))
			{
				return false;
			}
			OutItemId = ItemIds[0];
			return true;
		}

		bool AssertConsistentResult(FAutomationTestBase& Test, const FSurvivalRuntimeSpawnResult& Result, const TCHAR* Label)
		{
			if (Result.bSucceeded)
			{
				return Test.TestEqual(*FString::Printf(TEXT("%s success code"), Label), Result.ResultCode,
					ESurvivalRuntimeSpawnResultCode::Succeeded)
					&& Test.TestTrue(*FString::Printf(TEXT("%s success Actor"), Label), IsValid(Result.SpawnedActor))
					&& Test.TestTrue(*FString::Printf(TEXT("%s success has no diagnostic"), Label), Result.FailureReason.IsEmpty());
			}

			return Test.TestTrue(*FString::Printf(TEXT("%s failed result has no Actor"), Label), Result.SpawnedActor == nullptr)
				&& Test.TestTrue(*FString::Printf(TEXT("%s failure code is not success"), Label),
					Result.ResultCode != ESurvivalRuntimeSpawnResultCode::Succeeded)
				&& Test.TestFalse(*FString::Printf(TEXT("%s failure has a diagnostic"), Label), Result.FailureReason.IsEmpty());
		}

		bool AreSameStacks(const TArray<FItemStack>& Left, const TArray<FItemStack>& Right)
		{
			if (Left.Num() != Right.Num())
			{
				return false;
			}
			for (int32 Index = 0; Index < Left.Num(); ++Index)
			{
				if (Left[Index].SlotId != Right[Index].SlotId || Left[Index].ItemId != Right[Index].ItemId
					|| Left[Index].Quantity != Right[Index].Quantity)
				{
					return false;
				}
			}
			return true;
		}

		int32 FindSurvivalSlotId(const UPackageComponent* Package, int32 ItemId)
		{
			if (!Package)
			{
				return INDEX_NONE;
			}

			TArray<FSurvivalItemView> Items;
			Package->GetSurvivalInventoryItems(Items);
			for (const FSurvivalItemView& Item : Items)
			{
				if (Item.Stack.ItemId == ItemId && Item.Stack.Quantity > 0)
				{
					return Item.Stack.SlotId;
				}
			}
			return INDEX_NONE;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalTaggedInventoryConsumptionTest,
		"LegoGame.Survival.Core.Inventory.TaggedConsumption",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalTaggedInventoryConsumptionTest::RunTest(const FString& Parameters)
	{
		FRuntimeTestWorld TestWorld;
		if (!TestNotNull(TEXT("Runtime test world"), TestWorld.World)
			|| !TestNotNull(TEXT("Runtime provider"), TestWorld.Provider))
		{
			return false;
		}

		const FGameplayTag FoodTag = FGameplayTag::RequestGameplayTag(TEXT("Item.Category.Food"));
		const FGameplayTag WaterTag = FGameplayTag::RequestGameplayTag(TEXT("Item.Category.Water"));
		const FGameplayTag ParentTag = FGameplayTag::RequestGameplayTag(TEXT("Item.Category"));
		int32 FoodItemId = INDEX_NONE;
		int32 WaterItemId = INDEX_NONE;
		if (!FindTaggedItem(*this, TestWorld.GetProps(), FoodTag, FoodItemId)
			|| !FindTaggedItem(*this, TestWorld.GetProps(), WaterTag, WaterItemId))
		{
			return false;
		}

		ALgCharacterBase* Character = TestWorld.World->SpawnActor<ALgCharacterBase>();
		UPackageComponent* Package = Character ? Character->GetPackageComponent() : nullptr;
		if (!TestNotNull(TEXT("Authority inventory owner"), Character) || !TestNotNull(TEXT("Inventory component"), Package))
		{
			return false;
		}

		FItemStack FoodStack;
		FoodStack.ItemId = FoodItemId;
		FoodStack.Quantity = 1;
		FItemStack WaterStack;
		WaterStack.ItemId = WaterItemId;
		WaterStack.Quantity = 2;
		if (!TestTrue(TEXT("Food stack is added"), Package->TryAddItemStack(FoodStack))
			|| !TestTrue(TEXT("Water stack is added"), Package->TryAddItemStack(WaterStack)))
		{
			return false;
		}

		TestEqual(TEXT("Invalid tag query is zero"), Package->GetItemQuantityByTag(FGameplayTag()), 0);
		TestEqual(TEXT("Child Food tag query totals Food"), Package->GetItemQuantityByTag(FoodTag), 1);
		TestEqual(TEXT("Parent Item.Category query matches child Food and Water"), Package->GetItemQuantityByTag(ParentTag), 3);

		const TArray<FItemStack> BeforeFailedConsumption = Package->GetSurvivalItemStacks();
		TestFalse(TEXT("Insufficient tagged consumption is rejected"), Package->TryConsumeItemsByTag(ParentTag, 4));
		TestTrue(TEXT("Insufficient consumption preserves each stack"), AreSameStacks(Package->GetSurvivalItemStacks(), BeforeFailedConsumption));
		TestFalse(TEXT("Zero quantity is rejected"), Package->TryConsumeItemsByTag(ParentTag, 0));
		TestTrue(TEXT("Invalid quantity preserves stacks"), AreSameStacks(Package->GetSurvivalItemStacks(), BeforeFailedConsumption));

		int32 NotificationCount = 0;
		const FDelegateHandle NotificationHandle = Package->OnSurvivalInventoryChanged.AddLambda(
			[&NotificationCount](const TArray<FItemStack>&) { ++NotificationCount; });
		TestTrue(TEXT("Cross-slot parent-tag consumption succeeds"), Package->TryConsumeItemsByTag(ParentTag, 2));
		Package->OnSurvivalInventoryChanged.Remove(NotificationHandle);
		TestEqual(TEXT("Successful atomic consumption sends one notification"), NotificationCount, 1);
		TestEqual(TEXT("SlotId order consumes Food before Water"), Package->GetItemQuantityByTag(FoodTag), 0);
		TestEqual(TEXT("Only one Water remains after stable cross-slot consume"), Package->GetItemQuantityByTag(WaterTag), 1);

		Character->SetRole(ROLE_SimulatedProxy);
		TestFalse(TEXT("Non-authority tagged consumption is rejected"), Package->TryConsumeItemsByTag(ParentTag, 1));
		Character->SetRole(ROLE_Authority);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalConsumableUseTest,
		"LegoGame.Survival.Core.Inventory.ConsumableUse",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalConsumableUseTest::RunTest(const FString& Parameters)
	{
		FRuntimeTestWorld TestWorld;
		if (!TestNotNull(TEXT("Consumable test world"), TestWorld.World))
		{
			return false;
		}

		int32 FoodItemId = INDEX_NONE;
		int32 WaterItemId = INDEX_NONE;
		int32 MaterialItemId = INDEX_NONE;
		if (!FindTaggedItem(*this, TestWorld.GetProps(), LG::SurvivalTags::Item_Food, FoodItemId)
			|| !FindTaggedItem(*this, TestWorld.GetProps(), LG::SurvivalTags::Item_Water, WaterItemId)
			|| !FindTaggedItem(*this, TestWorld.GetProps(), LG::SurvivalTags::Item_Material, MaterialItemId))
		{
			return false;
		}

		ALgCharacterBase* Character = TestWorld.World->SpawnActor<ALgCharacterBase>();
		UPackageComponent* Package = Character ? Character->GetPackageComponent() : nullptr;
		USurvivalVitalsComponent* Vitals = Character ? Character->GetSurvivalVitalsComponent() : nullptr;
		if (!TestNotNull(TEXT("Consumable inventory owner"), Character)
			|| !TestNotNull(TEXT("Consumable inventory component"), Package)
			|| !TestNotNull(TEXT("Consumable vitals component"), Vitals))
		{
			return false;
		}
		if (!Character->HasActorBegunPlay())
		{
			Character->DispatchBeginPlay();
		}

		FItemStack FoodStack;
		FoodStack.ItemId = FoodItemId;
		FoodStack.Quantity = 2;
		FItemStack WaterStack;
		WaterStack.ItemId = WaterItemId;
		WaterStack.Quantity = 2;
		FItemStack MaterialStack;
		MaterialStack.ItemId = MaterialItemId;
		MaterialStack.Quantity = 1;
		if (!TestTrue(TEXT("Food consumable stack is added"), Package->TryAddItemStack(FoodStack))
			|| !TestTrue(TEXT("Water consumable stack is added"), Package->TryAddItemStack(WaterStack))
			|| !TestTrue(TEXT("Non-consumable stack is added"), Package->TryAddItemStack(MaterialStack)))
		{
			return false;
		}

		const int32 FoodSlotId = FindSurvivalSlotId(Package, FoodItemId);
		const int32 WaterSlotId = FindSurvivalSlotId(Package, WaterItemId);
		const int32 MaterialSlotId = FindSurvivalSlotId(Package, MaterialItemId);
		if (!TestTrue(TEXT("Food consumable stack has a valid slot"), FoodSlotId != INDEX_NONE)
			|| !TestTrue(TEXT("Water consumable stack has a valid slot"), WaterSlotId != INDEX_NONE)
			|| !TestTrue(TEXT("Non-consumable stack has a valid slot"), MaterialSlotId != INDEX_NONE))
		{
			return false;
		}

		int32 NotificationCount = 0;
		const FDelegateHandle NotificationHandle = Package->OnSurvivalInventoryChanged.AddLambda(
			[&NotificationCount](const TArray<FItemStack>&) { ++NotificationCount; });

		Package->RequestConsumeItemStack(FoodSlotId, 1);
		TestEqual(TEXT("Full vitals reject direct consumable consumption"), Package->GetItemQuantityByTag(LG::SurvivalTags::Item_Food), 2);
		Package->RequestConsumeItemStack(WaterSlotId, 1);
		TestEqual(TEXT("Full thirst rejects direct water consumption"), Package->GetItemQuantityByTag(LG::SurvivalTags::Item_Water), 2);
		TestEqual(TEXT("Rejected full-vitals use sends no inventory notification"), NotificationCount, 0);

		float HealthRestore = 0.0f;
		float HungerRestore = 0.0f;
		float ThirstRestore = 0.0f;
		if (!TestTrue(TEXT("Food item has a configured consumable effect"),
			TestWorld.GetProps()->GetConsumableEffects(FoodItemId, HealthRestore, HungerRestore, ThirstRestore)))
		{
			Package->OnSurvivalInventoryChanged.Remove(NotificationHandle);
			return false;
		}
		if (HealthRestore > 0.0f)
		{
			Vitals->ApplyDamage(10.0f, nullptr, nullptr);
		}
		if (HungerRestore > 0.0f || ThirstRestore > 0.0f)
		{
			// Test worlds do not schedule ticks for actors spawned after their manual BeginPlay.
			// Invoke the component tick directly to create a real hunger/thirst deficit.
			Vitals->TickComponent(1.1f, LEVELTICK_All, nullptr);
		}
		const FSurvivalVitalsSnapshot BeforeUseSnapshot = Vitals->GetSnapshot();
		TestTrue(TEXT("Consumable test creates a matching vital deficit"),
			(HealthRestore > 0.0f && BeforeUseSnapshot.Health < BeforeUseSnapshot.MaxHealth)
			|| (HungerRestore > 0.0f && BeforeUseSnapshot.Hunger < BeforeUseSnapshot.MaxHunger)
			|| (ThirstRestore > 0.0f && BeforeUseSnapshot.Thirst < BeforeUseSnapshot.MaxThirst));
		UPackageListViewWidget* Widget = NewObject<UPackageListViewWidget>(GetTransientPackage());
		UPackageItemData* ItemData = NewObject<UPackageItemData>(GetTransientPackage());
		if (!TestNotNull(TEXT("Inventory list widget"), Widget)
			|| !TestNotNull(TEXT("Inventory list item data"), ItemData))
		{
			Package->OnSurvivalInventoryChanged.Remove(NotificationHandle);
			return false;
		}

		FPackageListViewWidgetTestAccess::ConfigureSurvivalStack(*ItemData, FoodItemId, FoodSlotId);
		TestTrue(TEXT("Double-click use accepts a valid consumable Survival stack"),
			FPackageListViewWidgetTestAccess::RequestUse(*Widget, *ItemData, Package));
		TestEqual(TEXT("Double-click use consumes exactly one item"), Package->GetItemQuantityByTag(LG::SurvivalTags::Item_Food), 1);
		TestEqual(TEXT("Successful direct use sends one inventory notification"), NotificationCount, 1);
		TestEqual(TEXT("Double-click selects its Survival stack"), Package->GetSelectedSurvivalSlotId(), FoodSlotId);

		FPackageListViewWidgetTestAccess::ConfigureSurvivalStack(*ItemData, WaterItemId, WaterSlotId);
		TestTrue(TEXT("Double-click use accepts the configured water Survival stack"),
			FPackageListViewWidgetTestAccess::RequestUse(*Widget, *ItemData, Package));
		TestEqual(TEXT("Double-click water use consumes exactly one item"), Package->GetItemQuantityByTag(LG::SurvivalTags::Item_Water), 1);
		TestEqual(TEXT("Water use sends one additional inventory notification"), NotificationCount, 2);
		TestEqual(TEXT("Double-click selects the water Survival stack"), Package->GetSelectedSurvivalSlotId(), WaterSlotId);

		FPackageListViewWidgetTestAccess::ConfigureSurvivalStack(*ItemData, MaterialItemId, MaterialSlotId);
		TestFalse(TEXT("Double-click rejects a non-consumable Survival stack"),
			FPackageListViewWidgetTestAccess::RequestUse(*Widget, *ItemData, Package));
		TestEqual(TEXT("Rejected non-consumable does not change inventory"), Package->GetItemQuantityByTag(LG::SurvivalTags::Item_Material), 1);
		TestEqual(TEXT("Rejected non-consumable sends no inventory notification"), NotificationCount, 2);

		FPackageListViewWidgetTestAccess::ConfigureNonSurvivalItem(*ItemData, FoodItemId);
		TestFalse(TEXT("Double-click rejects non-Survival inventory entries"),
			FPackageListViewWidgetTestAccess::RequestUse(*Widget, *ItemData, Package));
		Package->OnSurvivalInventoryChanged.Remove(NotificationHandle);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalRuntimeResourceSpawnTest,
		"LegoGame.Survival.Core.RuntimeSpawn.Resource",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalRuntimeResourceSpawnTest::RunTest(const FString& Parameters)
	{
		FRuntimeTestWorld TestWorld;
		if (!TestNotNull(TEXT("Runtime provider"), TestWorld.Provider))
		{
			return false;
		}

		FSurvivalResourceSpawnRequest InvalidRequest;
		const FSurvivalRuntimeSpawnResult InvalidResult =
			ISurvivalRuntimeSpawnInterface::Execute_TrySpawnResource(TestWorld.Provider, InvalidRequest);
		TestEqual(TEXT("Invalid resource request code"), InvalidResult.ResultCode, ESurvivalRuntimeSpawnResultCode::InvalidRequest);
		AssertConsistentResult(*this, InvalidResult, TEXT("Invalid resource request"));

		FSurvivalResourceSpawnRequest MissingRequest;
		MissingRequest.ItemTag = LG::SurvivalTags::Item_Medical;
		MissingRequest.Quantity = 1;
		const FSurvivalRuntimeSpawnResult MissingResult =
			ISurvivalRuntimeSpawnInterface::Execute_TrySpawnResource(TestWorld.Provider, MissingRequest);
		TestEqual(TEXT("Unconfigured resource definition code"), MissingResult.ResultCode, ESurvivalRuntimeSpawnResultCode::NoMatchingDefinition);
		AssertConsistentResult(*this, MissingResult, TEXT("Missing resource definition"));

		const FGameplayTag FoodTag = FGameplayTag::RequestGameplayTag(TEXT("Item.Category.Food"));
		int32 FoodItemId = INDEX_NONE;
		if (!FindTaggedItem(*this, TestWorld.GetProps(), FoodTag, FoodItemId))
		{
			return false;
		}
		FSurvivalResourceSpawnRequest SuccessRequest;
		SuccessRequest.ItemTag = FoodTag;
		SuccessRequest.Quantity = 3;
		SuccessRequest.SpawnTransform = FTransform(FRotator::ZeroRotator, FVector(1200.0f, 0.0f, 100.0f));
		const FSurvivalRuntimeSpawnResult SuccessResult =
			ISurvivalRuntimeSpawnInterface::Execute_TrySpawnResource(TestWorld.Provider, SuccessRequest);
		AssertConsistentResult(*this, SuccessResult, TEXT("Successful resource spawn"));
		ASceneItemActor* SceneItem = Cast<ASceneItemActor>(SuccessResult.SpawnedActor);
		TestNotNull(TEXT("Resource result is a real scene pickup"), SceneItem);
		if (SceneItem)
		{
			TestEqual(TEXT("Pickup ItemId follows stable resolver"), SceneItem->GetItemStack().ItemId, FoodItemId);
			TestEqual(TEXT("Pickup quantity is preserved"), SceneItem->GetItemStack().Quantity, 3);
			TestEqual(TEXT("Pickup slot is unowned"), SceneItem->GetItemStack().SlotId, INDEX_NONE);
			TestTrue(TEXT("Pickup is replicated"), SceneItem->GetIsReplicated());
			TestTrue(TEXT("Pickup transform matches request"), SceneItem->GetActorTransform().Equals(SuccessRequest.SpawnTransform));
		}

		const FGameplayTag AmmoTag = LG::SurvivalTags::Item_Ammo;
		int32 AmmoItemId = INDEX_NONE;
		if (!FindTaggedItem(*this, TestWorld.GetProps(), AmmoTag, AmmoItemId))
		{
			return false;
		}
		FSurvivalItemView AmmoDefinition;
		if (!TestTrue(TEXT("Ammo definition supports a thirty-round pickup"),
			TestWorld.GetProps()->GetSurvivalItemView(AmmoItemId, AmmoDefinition))
			|| !TestTrue(TEXT("Ammo stack limit is at least thirty rounds"), AmmoDefinition.MaxStackSize >= 30))
		{
			return false;
		}
		FSurvivalResourceSpawnRequest AmmoRequest;
		AmmoRequest.ItemTag = AmmoTag;
		AmmoRequest.Quantity = 30;
		AmmoRequest.SpawnTransform = FTransform(FRotator::ZeroRotator, FVector(1400.0f, 0.0f, 100.0f));
		const FSurvivalRuntimeSpawnResult AmmoResult =
			ISurvivalRuntimeSpawnInterface::Execute_TrySpawnResource(TestWorld.Provider, AmmoRequest);
		AssertConsistentResult(*this, AmmoResult, TEXT("Thirty-round ammo resource spawn"));
		ASceneItemActor* AmmoSceneItem = Cast<ASceneItemActor>(AmmoResult.SpawnedActor);
		if (!TestNotNull(TEXT("Ammo result is a real scene pickup"), AmmoSceneItem))
		{
			return false;
		}
		TestEqual(TEXT("Ammo pickup uses the resolved production ItemId"), AmmoSceneItem->GetItemStack().ItemId, AmmoItemId);
		TestEqual(TEXT("Ammo pickup represents thirty rounds"), AmmoSceneItem->GetItemStack().Quantity, 30);
		TestEqual(TEXT("Ammo pickup is unowned"), AmmoSceneItem->GetItemStack().SlotId, INDEX_NONE);
		TestTrue(TEXT("Ammo pickup is replicated"), AmmoSceneItem->GetIsReplicated());
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalRespawnEnergyProductionTest,
		"LegoGame.Survival.Core.RespawnEnergy.ProductionPipeline",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalRespawnEnergyProductionTest::RunTest(const FString& Parameters)
	{
		FRuntimeTestWorld TestWorld;
		if (!TestNotNull(TEXT("Runtime test world"), TestWorld.World)
			|| !TestNotNull(TEXT("Runtime provider"), TestWorld.Provider))
		{
			return false;
		}

		const FGameplayTag RespawnEnergyTag = LG::SurvivalTags::Item_RespawnEnergy;
		TestTrue(TEXT("RespawnEnergy is a registered formal gameplay tag"), RespawnEnergyTag.IsValid());

		int32 RespawnEnergyItemId = INDEX_NONE;
		if (!FindTaggedItem(*this, TestWorld.GetProps(), RespawnEnergyTag, RespawnEnergyItemId))
		{
			return false;
		}
		TestEqual(TEXT("RespawnEnergy uses its stable production item id"), RespawnEnergyItemId,
			LG::SurvivalItemIds::RespawnEnergy);

		FSurvivalItemView Definition;
		if (!TestTrue(TEXT("RespawnEnergy definition resolves from production props data"),
			TestWorld.GetProps()->GetSurvivalItemView(RespawnEnergyItemId, Definition)))
		{
			return false;
		}
		TestTrue(TEXT("RespawnEnergy definition keeps its category tag"), Definition.ItemTags.HasTagExact(RespawnEnergyTag));
		TestEqual(TEXT("RespawnEnergy definition has its production display name"),
			Definition.DisplayName.ToString(), FString(TEXT("Respawn Energy Cell")));
		TestEqual(TEXT("RespawnEnergy definition keeps its default pickup quantity"), Definition.Stack.Quantity, 1);
		TestEqual(TEXT("RespawnEnergy definition has a production stack limit"), Definition.MaxStackSize, 10);
		if (Definition.Icon.IsNull())
		{
			AddWarning(TEXT("Content blocker: RespawnEnergy Icon is intentionally unset pending approved production art."));
		}
		const FSkinHeader* RawDefinition = static_cast<const FSkinHeader*>(TestWorld.GetProps()->GetPropsById(RespawnEnergyItemId));
		if (RawDefinition && !RawDefinition->StaticMesh && !RawDefinition->SkeletalMesh)
		{
			AddWarning(TEXT("Content blocker: RespawnEnergy Mesh is intentionally unset pending approved production art."));
		}

		FSurvivalResourceSpawnRequest SpawnRequest;
		SpawnRequest.ItemTag = RespawnEnergyTag;
		SpawnRequest.Quantity = 3;
		SpawnRequest.SpawnTransform = FTransform(FRotator::ZeroRotator, FVector(120.0f, 0.0f, 100.0f));
		const FSurvivalRuntimeSpawnResult SpawnResult =
			ISurvivalRuntimeSpawnInterface::Execute_TrySpawnResource(TestWorld.Provider, SpawnRequest);
		if (!AssertConsistentResult(*this, SpawnResult, TEXT("RespawnEnergy production spawn")))
		{
			return false;
		}

		ASceneItemActor* SceneItem = Cast<ASceneItemActor>(SpawnResult.SpawnedActor);
		if (!TestNotNull(TEXT("RespawnEnergy spawn is a real replicated pickup actor"), SceneItem))
		{
			return false;
		}
		TestTrue(TEXT("RespawnEnergy pickup replication is enabled"), SceneItem->GetIsReplicated());
		TestEqual(TEXT("RespawnEnergy pickup uses resolved production id"), SceneItem->GetItemStack().ItemId, RespawnEnergyItemId);
		TestEqual(TEXT("RespawnEnergy pickup preserves requested quantity"), SceneItem->GetItemStack().Quantity, 3);

		ALgCharacterBase* Character = TestWorld.World->SpawnActor<ALgCharacterBase>(
			ALgCharacterBase::StaticClass(), FTransform(FRotator::ZeroRotator, FVector::ZeroVector));
		UPackageComponent* Package = Character ? Character->GetPackageComponent() : nullptr;
		if (!TestNotNull(TEXT("RespawnEnergy pickup has an authority character"), Character)
			|| !TestNotNull(TEXT("RespawnEnergy pickup has an inventory"), Package))
		{
			return false;
		}

		ISurvivalInteractableInterface::Execute_Interact(SceneItem, Character);
		TestFalse(TEXT("Successful RespawnEnergy pickup consumes the scene actor"), IsValid(SceneItem));
		TestEqual(TEXT("Picked-up RespawnEnergy is queryable by formal tag"), Package->GetItemQuantityByTag(RespawnEnergyTag), 3);
		TestTrue(TEXT("RespawnEnergy tagged consumption succeeds when sufficient"),
			Package->TryConsumeItemsByTag(RespawnEnergyTag, 2));
		TestEqual(TEXT("Successful tagged consumption reduces RespawnEnergy exactly"),
			Package->GetItemQuantityByTag(RespawnEnergyTag), 1);

		const TArray<FItemStack> BeforeInsufficientConsumption = Package->GetSurvivalItemStacks();
		TestFalse(TEXT("Insufficient RespawnEnergy consumption is rejected atomically"),
			Package->TryConsumeItemsByTag(RespawnEnergyTag, 2));
		TestTrue(TEXT("Insufficient RespawnEnergy consumption leaves inventory unchanged"),
			AreSameStacks(Package->GetSurvivalItemStacks(), BeforeInsufficientConsumption));
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalWeaponProductionPipelineTest,
		"LegoGame.Survival.Core.Weapon.ProductionPipeline",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalWeaponProductionPipelineTest::RunTest(const FString& Parameters)
	{
		FRuntimeTestWorld TestWorld;
		if (!TestNotNull(TEXT("Runtime test world"), TestWorld.World)
			|| !TestNotNull(TEXT("Runtime provider"), TestWorld.Provider)
			|| !TestNotNull(TEXT("Props subsystem"), TestWorld.GetProps()))
		{
			return false;
		}

		const FGameplayTag WeaponTag = LG::SurvivalTags::Item_Weapon;
		TArray<int32> WeaponIds;
		TestWorld.GetProps()->GetSurvivalItemIdsByTag(WeaponTag, WeaponIds);
		if (!TestEqual(TEXT("Weapon tag resolves one production weapon"), WeaponIds.Num(), 1))
		{
			return false;
		}
		const int32 WeaponItemId = WeaponIds[0];
		TestEqual(TEXT("Production weapon keeps its stable ItemId"), WeaponItemId, WEAPON_INDEX);
		TestTrue(TEXT("Production weapon is formally classified"), TestWorld.GetProps()->IsSurvivalWeaponItem(WeaponItemId));
		TestFalse(TEXT("Production weapon is never classified as a stackable resource"),
			TestWorld.GetProps()->IsStackableSurvivalResourceItem(WeaponItemId));
		TestTrue(TEXT("Production weapon has all required assets and ammo route"),
			TestWorld.GetProps()->IsValidSurvivalWeaponDefinition(WeaponItemId));
		const FWeaponBaseHeader* WeaponDefinition = static_cast<const FWeaponBaseHeader*>(TestWorld.GetProps()->GetPropsById(WeaponItemId));
		if (!TestNotNull(TEXT("Production weapon definition"), WeaponDefinition))
		{
			return false;
		}
		TestNotNull(TEXT("Production weapon class"), WeaponDefinition->WeaponClass.Get());
		TestNotNull(TEXT("Production weapon skeletal mesh"), WeaponDefinition->SkeletalMesh.Get());
		TestNotNull(TEXT("Production weapon icon"), WeaponDefinition->Icon);
		TestEqual(TEXT("Production weapon uses configured ammo ItemId"), WeaponDefinition->AmmoItemId, 203);

		TestWorld.Provider->SetRole(ROLE_SimulatedProxy);
		FSurvivalResourceSpawnRequest AuthorityRequest;
		AuthorityRequest.ItemTag = WeaponTag;
		AuthorityRequest.Quantity = 1;
		const FSurvivalRuntimeSpawnResult NonAuthorityResult =
			ISurvivalRuntimeSpawnInterface::Execute_TrySpawnResource(TestWorld.Provider, AuthorityRequest);
		TestEqual(TEXT("Non-authority weapon spawn is rejected"), NonAuthorityResult.ResultCode,
			ESurvivalRuntimeSpawnResultCode::RejectedNotAuthority);
		TestWorld.Provider->SetRole(ROLE_Authority);

		FSurvivalResourceSpawnRequest InvalidQuantityRequest;
		InvalidQuantityRequest.ItemTag = WeaponTag;
		InvalidQuantityRequest.Quantity = 2;
		const FSurvivalRuntimeSpawnResult InvalidQuantityResult =
			ISurvivalRuntimeSpawnInterface::Execute_TrySpawnResource(TestWorld.Provider, InvalidQuantityRequest);
		TestEqual(TEXT("Weapon spawn rejects non-singleton quantities"), InvalidQuantityResult.ResultCode,
			ESurvivalRuntimeSpawnResultCode::InvalidRequest);

		ALgCharacterBase* Character = TestWorld.World->SpawnActor<ALgCharacterBase>(
			ALgCharacterBase::StaticClass(), FTransform(FRotator::ZeroRotator, FVector::ZeroVector));
		UPackageComponent* Package = Character ? Character->GetPackageComponent() : nullptr;
		if (!TestNotNull(TEXT("Weapon pickup authority character"), Character)
			|| !TestNotNull(TEXT("Weapon pickup package"), Package))
		{
			return false;
		}

		FSurvivalResourceSpawnRequest SpawnRequest;
		SpawnRequest.ItemTag = WeaponTag;
		SpawnRequest.Quantity = 1;
		SpawnRequest.SpawnTransform = FTransform(FRotator::ZeroRotator, FVector(100.0f, 0.0f, 100.0f));
		const FSurvivalRuntimeSpawnResult SpawnResult =
			ISurvivalRuntimeSpawnInterface::Execute_TrySpawnResource(TestWorld.Provider, SpawnRequest);
		ASceneItemActor* SceneItem = Cast<ASceneItemActor>(SpawnResult.SpawnedActor);
		if (!TestTrue(TEXT("Authority weapon spawn succeeds"), SpawnResult.bSucceeded)
			|| !TestNotNull(TEXT("Weapon spawn is a real scene pickup"), SceneItem))
		{
			return false;
		}
		TestTrue(TEXT("Weapon pickup is replicated"), SceneItem->GetIsReplicated());
		TestEqual(TEXT("Weapon pickup stack has the formal ItemId"), SceneItem->GetItemStack().ItemId, WeaponItemId);
		TestEqual(TEXT("Weapon pickup stack is singleton"), SceneItem->GetItemStack().Quantity, 1);
		TestEqual(TEXT("Weapon pickup has no inventory slot"), SceneItem->GetItemStack().SlotId, INDEX_NONE);

		ISurvivalInteractableInterface::Execute_Interact(SceneItem, Character);
		TestFalse(TEXT("Successful weapon pickup destroys the world actor"), IsValid(SceneItem));
		TestFalse(TEXT("Weapon pickup never enters stackable Survival inventory"),
			Package->GetSurvivalItemStacks().ContainsByPredicate([WeaponItemId](const FItemStack& Stack)
			{
				return Stack.ItemId == WeaponItemId;
			}));

		int32 WeaponPackageKey = INDEX_NONE;
		for (const TPair<int32, int32>& Item : Package->GetPackageItems())
		{
			if (Item.Value == WeaponItemId)
			{
				WeaponPackageKey = Item.Key;
				break;
			}
		}
		if (!TestTrue(TEXT("Weapon pickup enters equipable package inventory"), WeaponPackageKey != INDEX_NONE))
		{
			return false;
		}

		Package->EquipWeaponFromPackage(WeaponPackageKey);
		AWeaponBase* EquippedWeapon = Package->GetHoldWeapon();
		if (!TestNotNull(TEXT("Weapon equips as a real WeaponBase"), EquippedWeapon))
		{
			return false;
		}
		TestEqual(TEXT("Equipped weapon keeps its ItemId"), EquippedWeapon->GetID(), WeaponItemId);
		TestEqual(TEXT("Equipped weapon owner is the inventory character"), EquippedWeapon->GetOwner(), static_cast<AActor*>(Character));
		TestEqual(TEXT("Equipped weapon resolves its configured ammo ItemId"), EquippedWeapon->GetAmmoItemId(), 203);
		TestFalse(TEXT("Equipping removes the source package entry"), Package->GetPackageItems().Contains(WeaponPackageKey));
		TestTrue(TEXT("Equipped character exposes the public weapon state interface"),
			Character->GetClass()->ImplementsInterface(USurvivalWeaponStateInterface::StaticClass()));
		const FSurvivalWeaponAmmoSnapshot AmmoSnapshot =
			ISurvivalWeaponStateInterface::Execute_GetSurvivalWeaponAmmoSnapshot(Character);
		TestTrue(TEXT("Weapon snapshot reports an equipped weapon"), AmmoSnapshot.bHasEquippedWeapon);
		TestEqual(TEXT("Weapon snapshot reports loaded rounds"), AmmoSnapshot.LoadedAmmo, EquippedWeapon->GetCurrentClipVolume());
		TestEqual(TEXT("Weapon snapshot reports clip capacity"), AmmoSnapshot.ClipCapacity, EquippedWeapon->GetMaxClipVolume());
		TestEqual(TEXT("Weapon snapshot reports reserve rounds"), AmmoSnapshot.ReserveAmmo, Package->GetItemQuantityById(203));

		Package->UnEquipWeaponToScene();
		TestTrue(TEXT("Unequipping clears held weapon after a successful drop"), Package->GetHoldWeapon() == nullptr);
		const FSurvivalWeaponAmmoSnapshot UnequippedSnapshot =
			ISurvivalWeaponStateInterface::Execute_GetSurvivalWeaponAmmoSnapshot(Character);
		TestFalse(TEXT("Unequipped weapon snapshot returns to its safe default"), UnequippedSnapshot.bHasEquippedWeapon);
		TestEqual(TEXT("Unequipped weapon snapshot has no loaded rounds"), UnequippedSnapshot.LoadedAmmo, 0);
		TestEqual(TEXT("Unequipped weapon snapshot has no clip capacity"), UnequippedSnapshot.ClipCapacity, 0);
		bool bFoundDroppedWeapon = false;
		for (TActorIterator<ASceneItemActor> It(TestWorld.World); It; ++It)
		{
			bFoundDroppedWeapon |= It->GetItemStack().ItemId == WeaponItemId && It->GetItemStack().Quantity == 1;
		}
		TestTrue(TEXT("Unequipping restores a valid weapon world pickup"), bFoundDroppedWeapon);

		const FGameplayTag MaterialTag = LG::SurvivalTags::Item_Material;
		int32 MaterialItemId = INDEX_NONE;
		if (!FindTaggedItem(*this, TestWorld.GetProps(), MaterialTag, MaterialItemId))
		{
			return false;
		}
		FSurvivalResourceSpawnRequest MaterialSpawnRequest;
		MaterialSpawnRequest.ItemTag = MaterialTag;
		MaterialSpawnRequest.Quantity = 2;
		MaterialSpawnRequest.SpawnTransform = FTransform(FRotator::ZeroRotator, FVector(100.0f, 0.0f, 100.0f));
		const FSurvivalRuntimeSpawnResult MaterialSpawnResult =
			ISurvivalRuntimeSpawnInterface::Execute_TrySpawnResource(TestWorld.Provider, MaterialSpawnRequest);
		ASceneItemActor* MaterialSceneItem = Cast<ASceneItemActor>(MaterialSpawnResult.SpawnedActor);
		if (!TestTrue(TEXT("Normal resource spawn succeeds alongside weapon support"), MaterialSpawnResult.bSucceeded)
			|| !TestNotNull(TEXT("Normal resource uses a scene pickup"), MaterialSceneItem))
		{
			return false;
		}
		ISurvivalInteractableInterface::Execute_Interact(MaterialSceneItem, Character);
		if (!TestFalse(TEXT("Normal resource pickup still destroys its scene actor"), IsValid(MaterialSceneItem))
			|| !TestEqual(TEXT("Normal resource pickup still enters Survival stacks"), Package->GetItemQuantityById(MaterialItemId), 2))
		{
			return false;
		}

		int32 ResourceMirrorKey = INDEX_NONE;
		for (const TPair<int32, int32>& Item : Package->GetPackageItems())
		{
			if (Item.Value == MaterialItemId)
			{
				ResourceMirrorKey = Item.Key;
				break;
			}
		}
		Package->EquipWeaponFromPackage(ResourceMirrorKey);
		TestTrue(TEXT("Non-weapon resource cannot enter the weapon equip path"), Package->GetHoldWeapon() == nullptr);

		FSurvivalRecipeDefinition WeaponRecipe;
		WeaponRecipe.RecipeId = TEXT("AutomationWeaponRecipe");
		WeaponRecipe.Ingredients = { FItemStack{ INDEX_NONE, MaterialItemId, 2 } };
		WeaponRecipe.Results = { FItemStack{ INDEX_NONE, WeaponItemId, 1 } };
		TestTrue(TEXT("In-memory weapon craft atomically consumes materials and adds weapon"), Package->TryCraftRecipe(WeaponRecipe, 1));
		TestEqual(TEXT("Successful weapon craft consumes all material"), Package->GetItemQuantityById(MaterialItemId), 0);
		bool bCraftedWeaponInPackage = false;
		for (const TPair<int32, int32>& Item : Package->GetPackageItems())
		{
			bCraftedWeaponInPackage |= Item.Value == WeaponItemId;
		}
		TestTrue(TEXT("Successful weapon craft adds an equipable package entry"), bCraftedWeaponInPackage);

		const TArray<FItemStack> BeforeFailedCraftStacks = Package->GetSurvivalItemStacks();
		const TMap<int32, int32> BeforeFailedCraftPackage = Package->GetPackageItems();
		TestFalse(TEXT("Insufficient weapon craft material is rejected"), Package->TryCraftRecipe(WeaponRecipe, 1));
		TestTrue(TEXT("Failed weapon craft leaves resource stacks unchanged"),
			AreSameStacks(Package->GetSurvivalItemStacks(), BeforeFailedCraftStacks));
		TestTrue(TEXT("Failed weapon craft leaves equipable package unchanged"),
			Package->GetPackageItems().OrderIndependentCompareEqual(BeforeFailedCraftPackage));

		int32 WeaponPickupsBeforeDeathDrop = 0;
		for (TActorIterator<ASceneItemActor> It(TestWorld.World); It; ++It)
		{
			WeaponPickupsBeforeDeathDrop += It->GetItemStack().ItemId == WeaponItemId ? 1 : 0;
		}
		Package->DropAllSurvivalItems();
		int32 WeaponPickupsAfterDeathDrop = 0;
		for (TActorIterator<ASceneItemActor> It(TestWorld.World); It; ++It)
		{
			WeaponPickupsAfterDeathDrop += It->GetItemStack().ItemId == WeaponItemId ? 1 : 0;
		}
		TestEqual(TEXT("Death drop creates one world pickup for the crafted package weapon"),
			WeaponPickupsAfterDeathDrop, WeaponPickupsBeforeDeathDrop + 1);
		TestFalse(TEXT("Death drop removes crafted weapon from the package"),
			Package->GetPackageItems().FindKey(WeaponItemId) != nullptr);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalRuntimeEnemySpawnTest,
		"LegoGame.Survival.Core.RuntimeSpawn.Enemy",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalRuntimeEnemySpawnTest::RunTest(const FString& Parameters)
	{
		FRuntimeTestWorld TestWorld;
		if (!TestNotNull(TEXT("Runtime provider"), TestWorld.Provider))
		{
			return false;
		}

		FSurvivalEnemySpawnRequest InvalidRequest;
		InvalidRequest.DifficultyMultiplier = 0.0f;
		const FSurvivalRuntimeSpawnResult InvalidResult =
			ISurvivalRuntimeSpawnInterface::Execute_TrySpawnEnemy(TestWorld.Provider, InvalidRequest);
		TestEqual(TEXT("Invalid difficulty code"), InvalidResult.ResultCode, ESurvivalRuntimeSpawnResultCode::InvalidRequest);
		AssertConsistentResult(*this, InvalidResult, TEXT("Invalid enemy request"));

		FSurvivalEnemySpawnRequest MissingRequest;
		const FSurvivalRuntimeSpawnResult MissingResult =
			ISurvivalRuntimeSpawnInterface::Execute_TrySpawnEnemy(TestWorld.Provider, MissingRequest);
		TestEqual(TEXT("Missing enemy class code"), MissingResult.ResultCode, ESurvivalRuntimeSpawnResultCode::NoMatchingDefinition);
		AssertConsistentResult(*this, MissingResult, TEXT("Missing enemy definition"));

		TestWorld.Provider->DefaultEnemyClass = AEnemyCharacter::StaticClass();
		TestWorld.Provider->DefaultEnemyArchetypeTag = FGameplayTag::RequestGameplayTag(TEXT("Item.Category.Food"));
		FSurvivalEnemySpawnRequest SuccessRequest;
		SuccessRequest.DifficultyMultiplier = 2.0f;
		SuccessRequest.SpawnTransform = FTransform(FRotator::ZeroRotator, FVector(1800.0f, 0.0f, 100.0f));
		const FSurvivalRuntimeSpawnResult SuccessResult =
			ISurvivalRuntimeSpawnInterface::Execute_TrySpawnEnemy(TestWorld.Provider, SuccessRequest);
		AssertConsistentResult(*this, SuccessResult, TEXT("Successful enemy spawn"));
		AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(SuccessResult.SpawnedActor);
		TestNotNull(TEXT("Enemy result is a real enemy Character"), Enemy);
		if (Enemy)
		{
			// UWorld::CreateWorld test worlds do not automatically dispatch BeginPlay
			// for actors spawned after their manual BeginPlay call. A running map does.
			if (!Enemy->HasActorBegunPlay())
			{
				Enemy->DispatchBeginPlay();
			}
			const FSurvivalVitalsSnapshot Snapshot = Enemy->GetSurvivalVitalsComponent()->GetSnapshot();
			TestEqual(TEXT("Difficulty doubles real enemy MaxHealth"), Snapshot.MaxHealth, 200.0f);
			TestEqual(TEXT("Difficulty doubles real enemy initial Health"), Snapshot.Health, 200.0f);
			TestTrue(TEXT("Enemy Actor replication remains enabled"), Enemy->GetIsReplicated());
			TestTrue(TEXT("Enemy movement replication remains enabled"), Enemy->IsReplicatingMovement());
			TestTrue(TEXT("Enemy transform matches request"), Enemy->GetActorTransform().Equals(SuccessRequest.SpawnTransform));
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FSurvivalRuntimeSpawnAuthorityTest,
		"LegoGame.Survival.Core.RuntimeSpawn.Authority",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FSurvivalRuntimeSpawnAuthorityTest::RunTest(const FString& Parameters)
	{
		FRuntimeTestWorld TestWorld;
		if (!TestNotNull(TEXT("Runtime provider"), TestWorld.Provider))
		{
			return false;
		}

		TestWorld.Provider->SetRole(ROLE_SimulatedProxy);
		FSurvivalResourceSpawnRequest ResourceRequest;
		ResourceRequest.ItemTag = FGameplayTag::RequestGameplayTag(TEXT("Item.Category.Food"));
		ResourceRequest.Quantity = 1;
		const FSurvivalRuntimeSpawnResult ResourceResult =
			ISurvivalRuntimeSpawnInterface::Execute_TrySpawnResource(TestWorld.Provider, ResourceRequest);
		TestEqual(TEXT("Non-authority resource request is rejected"), ResourceResult.ResultCode,
			ESurvivalRuntimeSpawnResultCode::RejectedNotAuthority);
		AssertConsistentResult(*this, ResourceResult, TEXT("Non-authority resource request"));

		FSurvivalEnemySpawnRequest EnemyRequest;
		const FSurvivalRuntimeSpawnResult EnemyResult =
			ISurvivalRuntimeSpawnInterface::Execute_TrySpawnEnemy(TestWorld.Provider, EnemyRequest);
		TestEqual(TEXT("Non-authority enemy request is rejected"), EnemyResult.ResultCode,
			ESurvivalRuntimeSpawnResultCode::RejectedNotAuthority);
		AssertConsistentResult(*this, EnemyResult, TEXT("Non-authority enemy request"));
		return true;
	}
}

#endif
