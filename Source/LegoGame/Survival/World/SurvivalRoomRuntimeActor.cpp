#include "SurvivalRoomRuntimeActor.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/RectLightComponent.h"
#include "Engine/StaticMesh.h"
#include "LegoGame/Survival/Contracts/SurvivalGameplayTags.h"
#include "Materials/MaterialInterface.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float BackroomsModuleSize = 300.0f;
	constexpr float BackroomsCeilingHeight = 300.0f;
	constexpr float BackroomsLampCenterOffset = 5.0593f;
	constexpr float BackroomsVentCenterOffset = 18.7061f;
	constexpr float BackroomsRectLightIntensity = 1050.0f;
	constexpr float BackroomsCeilingBackingBottom = 312.0f;
	constexpr float BackroomsCeilingBackingThickness = 20.0f;

	enum class EBackroomsRoomArchetype : uint8
	{
		Hub,
		StraightCorridor,
		CornerCorridor,
		OpenPillarHall,
		PartitionedHall,
		ResourceWorkshop,
		DarkCorridor
	};

	EBackroomsRoomArchetype ResolveArchetype(const FName TemplateId, const FGameplayTag& RoomType)
	{
		if (TemplateId == TEXT("Survival.Base"))
		{
			return EBackroomsRoomArchetype::Hub;
		}
		if (TemplateId == TEXT("Survival.NormalStraight"))
		{
			return EBackroomsRoomArchetype::StraightCorridor;
		}
		if (TemplateId == TEXT("Survival.NormalCorner"))
		{
			return EBackroomsRoomArchetype::CornerCorridor;
		}
		if (TemplateId == TEXT("Survival.NormalHall") || TemplateId == TEXT("Survival.NormalPillarHall"))
		{
			return EBackroomsRoomArchetype::OpenPillarHall;
		}
		if (TemplateId == TEXT("Survival.NormalPartitioned") || RoomType == LG::SurvivalTags::Room_Type_Monster)
		{
			return EBackroomsRoomArchetype::PartitionedHall;
		}
		if (TemplateId == TEXT("Survival.DarkCorridor"))
		{
			return EBackroomsRoomArchetype::DarkCorridor;
		}
		if (RoomType == LG::SurvivalTags::Room_Type_HighResource)
		{
			return EBackroomsRoomArchetype::ResourceWorkshop;
		}
		return EBackroomsRoomArchetype::OpenPillarHall;
	}
}

ASurvivalRoomRuntimeActor::ASurvivalRoomRuntimeActor()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;
	NavigationFloor = CreateDefaultSubobject<UBoxComponent>(TEXT("NavigationFloor"));
	NavigationFloor->SetupAttachment(SceneRoot);
	NavigationFloor->SetMobility(EComponentMobility::Movable);
	NavigationFloor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NavigationFloor->SetGenerateOverlapEvents(false);
	NavigationFloor->SetCanEverAffectNavigation(false);
	FloorGeometry = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FloorGeometry"));
	CeilingGeometry = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CeilingGeometry"));
	LampCeilingGeometry = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("LampCeilingGeometry"));
	WallGeometry = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WallGeometry"));
	LampGeometry = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("LampGeometry"));
	PillarGeometry = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PillarGeometry"));
	VentGeometry = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("VentGeometry"));
	CeilingBackingGeometry = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CeilingBackingGeometry"));
	GeneratedGeometry = {
		FloorGeometry,
		CeilingGeometry,
		LampCeilingGeometry,
		WallGeometry,
		LampGeometry,
		PillarGeometry,
		VentGeometry,
		CeilingBackingGeometry
	};
	for (UInstancedStaticMeshComponent* Component : GeneratedGeometry)
	{
		Component->SetupAttachment(SceneRoot);
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetCastShadow(true);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCanEverAffectNavigation(false);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		BlockMesh = CubeMesh.Object;
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> FloorAsset(TEXT("/Game/BackRooms/backrooms1_floor_tile_flat.backrooms1_floor_tile_flat"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> WallAsset(TEXT("/Game/BackRooms/backrooms1_wall_straight.backrooms1_wall_straight"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CeilingAsset(TEXT("/Game/BackRooms/backrooms1_ceiling_panel2.backrooms1_ceiling_panel2"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> LampCeilingAsset(TEXT("/Game/BackRooms/backrooms1_ceiling_panel.backrooms1_ceiling_panel"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> LampAsset(TEXT("/Game/BackRooms/backrooms1_ceiling_lamp.backrooms1_ceiling_lamp"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PillarAsset(TEXT("/Game/BackRooms/backrooms1_pillar2_1.backrooms1_pillar2_1"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> VentAsset(TEXT("/Game/BackRooms/backrooms1_ceiling_vent.backrooms1_ceiling_vent"));
	FloorMesh = FloorAsset.Succeeded() ? FloorAsset.Object : BlockMesh;
	WallMesh = WallAsset.Succeeded() ? WallAsset.Object : BlockMesh;
	CeilingMesh = CeilingAsset.Succeeded() ? CeilingAsset.Object : BlockMesh;
	LampCeilingMesh = LampCeilingAsset.Succeeded() ? LampCeilingAsset.Object : CeilingMesh;
	LampMesh = LampAsset.Succeeded() ? LampAsset.Object : BlockMesh;
	PillarMesh = PillarAsset.Succeeded() ? PillarAsset.Object : BlockMesh;
	VentMesh = VentAsset.Succeeded() ? VentAsset.Object : BlockMesh;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CarpetAsset(TEXT("/Game/LegoGame/Survival/Materials/M_Survival_Backrooms_Carpet.M_Survival_Backrooms_Carpet"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> LinoleumAsset(TEXT("/Game/LegoGame/Survival/Materials/M_Survival_Backrooms_Linoleum.M_Survival_Backrooms_Linoleum"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CeilingPanelsAsset(TEXT("/Game/LegoGame/Survival/Materials/M_Survival_Backrooms_CeilingPanels.M_Survival_Backrooms_CeilingPanels"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CeilingFrameAsset(TEXT("/Game/LegoGame/Survival/Materials/M_Survival_Backrooms_CeilingFrame.M_Survival_Backrooms_CeilingFrame"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> LampMaterialAsset(TEXT("/Game/LegoGame/Survival/Materials/M_Survival_Backrooms_Lamp.M_Survival_Backrooms_Lamp"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WallpaperAsset(TEXT("/Game/LegoGame/Survival/Materials/M_Survival_Backrooms_Wallpaper.M_Survival_Backrooms_Wallpaper"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlasterAsset(TEXT("/Game/LegoGame/Survival/Materials/M_Survival_Backrooms_Plaster.M_Survival_Backrooms_Plaster"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlasterWhiteAsset(TEXT("/Game/LegoGame/Survival/Materials/M_Survival_Backrooms_PlasterWhite.M_Survival_Backrooms_PlasterWhite"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WallTrimAsset(TEXT("/Game/LegoGame/Survival/Materials/M_Survival_Backrooms_WallTrim.M_Survival_Backrooms_WallTrim"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WallTrimDarkAsset(TEXT("/Game/LegoGame/Survival/Materials/M_Survival_Backrooms_WallTrimDark.M_Survival_Backrooms_WallTrimDark"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CeilingVentAsset(TEXT("/Game/LegoGame/Survival/Materials/M_Survival_Backrooms_CeilingVent.M_Survival_Backrooms_CeilingVent"));
	CarpetMaterial = CarpetAsset.Object;
	LinoleumMaterial = LinoleumAsset.Object;
	CeilingPanelsMaterial = CeilingPanelsAsset.Object;
	CeilingFrameMaterial = CeilingFrameAsset.Object;
	LampMaterial = LampMaterialAsset.Object;
	WallpaperMaterial = WallpaperAsset.Object;
	PlasterMaterial = PlasterAsset.Object;
	PlasterWhiteMaterial = PlasterWhiteAsset.Object;
	WallTrimMaterial = WallTrimAsset.Object;
	WallTrimDarkMaterial = WallTrimDarkAsset.Object;
	CeilingVentMaterial = CeilingVentAsset.Object;
}

void ASurvivalRoomRuntimeActor::InitializeRoom(const LG::Survival::World::FPlannedRoom& InPlan, const float InCellSize)
{
	if (!HasAuthority())
	{
		return;
	}
	RoomHandle = InPlan.Handle;
	TemplateId = InPlan.TemplateId;
	RoomTypeTag = InPlan.RoomType;
	Footprint = InPlan.Footprint;
	RotationQuarterTurns = InPlan.RotationQuarterTurns;
	UnlockPhaseIndex = InPlan.PhaseIndex;
	CellSize = InCellSize;
	RuntimeConnectors.Reset(InPlan.Connectors.Num());
	for (const LG::Survival::World::FPlannedConnector& Connector : InPlan.Connectors)
	{
		FRoomConnectorDefinition& RuntimeConnector = RuntimeConnectors.AddDefaulted_GetRef();
		RuntimeConnector.ConnectorId = Connector.ConnectorId;
		RuntimeConnector.Cell = Connector.Cell;
		RuntimeConnector.Direction = Connector.Direction;
		RuntimeConnector.ConnectorTags = Connector.Tags;
	}
	RoomState = ESurvivalRoomState::Locked;
	RebuildGeometry();
	ForceNetUpdate();
}

void ASurvivalRoomRuntimeActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshNavigationRegistration();
}

FRoomHandle ASurvivalRoomRuntimeActor::GetRoomHandle_Implementation() const
{
	return RoomHandle;
}

FGameplayTag ASurvivalRoomRuntimeActor::GetRoomTypeTag_Implementation() const
{
	return RoomTypeTag;
}

void ASurvivalRoomRuntimeActor::SetRoomState_Implementation(const ESurvivalRoomState NewState)
{
	if (!HasAuthority() || RoomState == NewState)
	{
		return;
	}
	RoomState = NewState;
	ForceNetUpdate();
}

void ASurvivalRoomRuntimeActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASurvivalRoomRuntimeActor, RoomHandle);
	DOREPLIFETIME(ASurvivalRoomRuntimeActor, TemplateId);
	DOREPLIFETIME(ASurvivalRoomRuntimeActor, RoomTypeTag);
	DOREPLIFETIME(ASurvivalRoomRuntimeActor, Footprint);
	DOREPLIFETIME(ASurvivalRoomRuntimeActor, RuntimeConnectors);
	DOREPLIFETIME(ASurvivalRoomRuntimeActor, RotationQuarterTurns);
	DOREPLIFETIME(ASurvivalRoomRuntimeActor, UnlockPhaseIndex);
	DOREPLIFETIME(ASurvivalRoomRuntimeActor, CellSize);
	DOREPLIFETIME(ASurvivalRoomRuntimeActor, RoomState);
}

void ASurvivalRoomRuntimeActor::OnRep_RoomLayout()
{
	RebuildGeometry();
}

void ASurvivalRoomRuntimeActor::OnRep_RoomState()
{
}

void ASurvivalRoomRuntimeActor::RebuildGeometry()
{
	NavigationFloor->SetCanEverAffectNavigation(false);
	NavigationFloor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NavigationFloor->SetBoxExtent(FVector::ZeroVector, false);
	for (UInstancedStaticMeshComponent* Component : GeneratedGeometry)
	{
		if (Component)
		{
			Component->ClearInstances();
			Component->EmptyOverrideMaterials();
			Component->SetStaticMesh(nullptr);
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Component->SetCanEverAffectNavigation(false);
		}
	}
	for (URectLightComponent* Light : GeneratedLights)
	{
		if (Light)
		{
			Light->DestroyComponent();
		}
	}
	GeneratedLights.Reset();
	if (!FloorMesh || !WallMesh || !CeilingMesh || !LampCeilingMesh || !LampMesh || !PillarMesh || !VentMesh
		|| Footprint.X <= 0 || Footprint.Y <= 0 || CellSize <= 0.0f)
	{
		return;
	}

	const EBackroomsRoomArchetype Archetype = ResolveArchetype(TemplateId, RoomTypeTag);
	const int32 ModulesPerCell = FMath::Max(1, FMath::RoundToInt(CellSize / BackroomsModuleSize));
	const float ModulePitch = CellSize / ModulesPerCell;
	const float ModuleScale = ModulePitch / BackroomsModuleSize;
	const float RoomWidth = Footprint.X * CellSize;
	const float RoomDepth = Footprint.Y * CellSize;
	const int32 QuarterModule = FMath::Clamp(ModulesPerCell / 4, 0, ModulesPerCell - 1);
	const int32 ThreeQuarterModule = FMath::Clamp(ModulesPerCell - 1 - QuarterModule, 0, ModulesPerCell - 1);
	const int32 CenterLowModule = FMath::Clamp(ModulesPerCell / 2 - 1, 0, ModulesPerCell - 1);
	const int32 CenterHighModule = FMath::Clamp(ModulesPerCell / 2, 0, ModulesPerCell - 1);
	const bool bHasNorth = RuntimeConnectors.ContainsByPredicate([](const FRoomConnectorDefinition& Connector) { return Connector.Direction == ERoomConnectorDirection::North; });
	const bool bHasEast = RuntimeConnectors.ContainsByPredicate([](const FRoomConnectorDefinition& Connector) { return Connector.Direction == ERoomConnectorDirection::East; });
	const bool bHasSouth = RuntimeConnectors.ContainsByPredicate([](const FRoomConnectorDefinition& Connector) { return Connector.Direction == ERoomConnectorDirection::South; });
	const bool bHasWest = RuntimeConnectors.ContainsByPredicate([](const FRoomConnectorDefinition& Connector) { return Connector.Direction == ERoomConnectorDirection::West; });
	const bool bVerticalRoute = (bHasNorth || bHasSouth) && !(bHasEast || bHasWest);

	// Imported Backrooms floor tiles are visually flat and do not contain reliable
	// simple navigation collision. A stable box supplies both the movement base and
	// an upward-facing surface for runtime Recast generation.
	constexpr float NavigationFloorThickness = 20.0f;
	NavigationFloor->SetRelativeLocation(FVector(RoomWidth * 0.5f, RoomDepth * 0.5f, -NavigationFloorThickness * 0.5f));
	NavigationFloor->SetBoxExtent(FVector(RoomWidth * 0.5f, RoomDepth * 0.5f, NavigationFloorThickness * 0.5f), false);
	NavigationFloor->SetCollisionProfileName(TEXT("BlockAll"));
	NavigationFloor->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	NavigationFloor->SetCanEverAffectNavigation(true);

	UInstancedStaticMeshComponent* FloorBatch = ConfigureGeometryBatch(FloorGeometry, FloorMesh, ECollisionEnabled::NoCollision, false);
	UInstancedStaticMeshComponent* CeilingBatch = ConfigureGeometryBatch(CeilingGeometry, CeilingMesh, ECollisionEnabled::NoCollision, false);
	UInstancedStaticMeshComponent* LampCeilingBatch = ConfigureGeometryBatch(LampCeilingGeometry, LampCeilingMesh, ECollisionEnabled::NoCollision, false);
	UInstancedStaticMeshComponent* WallBatch = ConfigureGeometryBatch(WallGeometry, WallMesh, ECollisionEnabled::QueryAndPhysics, true);
	UInstancedStaticMeshComponent* LampBatch = ConfigureGeometryBatch(LampGeometry, LampMesh, ECollisionEnabled::NoCollision, false);
	UInstancedStaticMeshComponent* PillarBatch = ConfigureGeometryBatch(PillarGeometry, PillarMesh, ECollisionEnabled::QueryAndPhysics, true);
	UInstancedStaticMeshComponent* VentBatch = ConfigureGeometryBatch(VentGeometry, VentMesh, ECollisionEnabled::NoCollision, false);
	UInstancedStaticMeshComponent* CeilingBackingBatch = ConfigureGeometryBatch(CeilingBackingGeometry, BlockMesh, ECollisionEnabled::QueryOnly, false);
	if (!FloorBatch || !CeilingBatch || !LampCeilingBatch || !WallBatch || !LampBatch || !PillarBatch || !VentBatch || !CeilingBackingBatch)
	{
		return;
	}

	// The visual ceiling meshes intentionally stay collision-free because their imported
	// simple collision is inconsistent. The continuous backing cube is a stable camera-only
	// blocker that keeps the spring arm inside the room without affecting pawns, weapon
	// traces, overlaps, or runtime navigation.
	CeilingBackingBatch->SetCollisionObjectType(ECC_WorldStatic);
	CeilingBackingBatch->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CeilingBackingBatch->SetCollisionResponseToAllChannels(ECR_Ignore);
	CeilingBackingBatch->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
	CeilingBackingBatch->SetGenerateOverlapEvents(false);

	if (Archetype == EBackroomsRoomArchetype::ResourceWorkshop && LinoleumMaterial)
	{
		FloorBatch->SetMaterial(0, LinoleumMaterial);
	}
	if (Archetype == EBackroomsRoomArchetype::ResourceWorkshop)
	{
		WallBatch->SetMaterial(0, PlasterWhiteMaterial ? PlasterWhiteMaterial.Get() : PlasterMaterial.Get());
		WallBatch->SetMaterial(1, PlasterWhiteMaterial ? PlasterWhiteMaterial.Get() : PlasterMaterial.Get());
		WallBatch->SetMaterial(2, WallTrimDarkMaterial ? WallTrimDarkMaterial.Get() : WallTrimMaterial.Get());
		PillarBatch->SetMaterial(0, PlasterWhiteMaterial ? PlasterWhiteMaterial.Get() : PlasterMaterial.Get());
		PillarBatch->SetMaterial(1, WallTrimDarkMaterial ? WallTrimDarkMaterial.Get() : WallTrimMaterial.Get());
	}
	else if (Archetype == EBackroomsRoomArchetype::PartitionedHall)
	{
		WallBatch->SetMaterial(0, PlasterMaterial);
		WallBatch->SetMaterial(1, PlasterMaterial);
		WallBatch->SetMaterial(2, WallTrimDarkMaterial ? WallTrimDarkMaterial.Get() : WallTrimMaterial.Get());
		PillarBatch->SetMaterial(0, PlasterMaterial);
		PillarBatch->SetMaterial(1, WallTrimDarkMaterial ? WallTrimDarkMaterial.Get() : WallTrimMaterial.Get());
	}
	else if (Archetype == EBackroomsRoomArchetype::DarkCorridor)
	{
		WallBatch->SetMaterial(2, WallTrimDarkMaterial ? WallTrimDarkMaterial.Get() : WallTrimMaterial.Get());
	}
	if (CeilingVentMaterial)
	{
		VentBatch->SetMaterial(0, CeilingVentMaterial);
	}
	if (CeilingFrameMaterial)
	{
		CeilingBackingBatch->SetMaterial(0, CeilingFrameMaterial);
	}

	const float BackingCenterZ = BackroomsCeilingBackingBottom + BackroomsCeilingBackingThickness * 0.5f;
	CeilingBackingBatch->AddInstance(FTransform(
		FRotator::ZeroRotator,
		FVector(RoomWidth * 0.5f, RoomDepth * 0.5f, BackingCenterZ),
		FVector((RoomWidth + 20.0f) / 100.0f, (RoomDepth + 20.0f) / 100.0f, BackroomsCeilingBackingThickness / 100.0f)));

	auto IsLampModule = [=, this](const int32 CellX, const int32 CellY, const int32 ModuleX, const int32 ModuleY)
	{
		const int32 GlobalModuleX = CellX * ModulesPerCell + ModuleX;
		const int32 GlobalModuleY = CellY * ModulesPerCell + ModuleY;
		const bool bQuarterX = ModuleX == QuarterModule || ModuleX == ThreeQuarterModule;
		const bool bQuarterY = ModuleY == QuarterModule || ModuleY == ThreeQuarterModule;
		switch (Archetype)
		{
		case EBackroomsRoomArchetype::Hub:
		case EBackroomsRoomArchetype::OpenPillarHall:
		case EBackroomsRoomArchetype::ResourceWorkshop:
			return bQuarterX && bQuarterY;
		case EBackroomsRoomArchetype::StraightCorridor:
			return bVerticalRoute
				? (ModuleX == CenterLowModule && bQuarterY)
				: (ModuleY == CenterLowModule && bQuarterX);
		case EBackroomsRoomArchetype::CornerCorridor:
			return ((bHasNorth || bHasSouth) && ModuleX == CenterLowModule && bQuarterY)
				|| ((bHasEast || bHasWest) && ModuleY == CenterLowModule && bQuarterX);
		case EBackroomsRoomArchetype::PartitionedHall:
			return bQuarterX && bQuarterY && ((GlobalModuleX + GlobalModuleY + RoomHandle.Value) & 1) == 0;
		case EBackroomsRoomArchetype::DarkCorridor:
			return bVerticalRoute
				? (ModuleX == CenterLowModule && ModuleY == (CellY & 1 ? QuarterModule : ThreeQuarterModule))
				: (ModuleY == CenterLowModule && ModuleX == (CellX & 1 ? QuarterModule : ThreeQuarterModule));
		default:
			return false;
		}
	};

	auto GetLightIntensityScale = [Archetype]()
	{
		switch (Archetype)
		{
		case EBackroomsRoomArchetype::Hub: return 0.85f;
		case EBackroomsRoomArchetype::StraightCorridor: return 0.65f;
		case EBackroomsRoomArchetype::CornerCorridor: return 0.70f;
		case EBackroomsRoomArchetype::OpenPillarHall: return 0.80f;
		case EBackroomsRoomArchetype::PartitionedHall: return 0.45f;
		case EBackroomsRoomArchetype::ResourceWorkshop: return 0.95f;
		case EBackroomsRoomArchetype::DarkCorridor: return 0.22f;
		default: return 0.75f;
		}
	};
	const float LightIntensityScale = GetLightIntensityScale();
	const FLinearColor RoomLightColor = Archetype == EBackroomsRoomArchetype::ResourceWorkshop
		? FLinearColor(1.0f, 0.95f, 0.82f)
		: FLinearColor(1.0f, 0.88f, 0.62f);

	for (int32 CellX = 0; CellX < Footprint.X; ++CellX)
	{
		for (int32 CellY = 0; CellY < Footprint.Y; ++CellY)
		{
			const FVector CellOrigin(CellX * CellSize, CellY * CellSize, 0.0f);
			for (int32 ModuleX = 0; ModuleX < ModulesPerCell; ++ModuleX)
			{
				for (int32 ModuleY = 0; ModuleY < ModulesPerCell; ++ModuleY)
				{
					const FVector TileLocation = CellOrigin + FVector((ModuleX + 0.5f) * ModulePitch, (ModuleY + 0.5f) * ModulePitch, 0.0f);
					FloorBatch->AddInstance(MakeBoundsCenteredTransform(FloorMesh, TileLocation, FRotator::ZeroRotator, FVector(ModuleScale, ModuleScale, 1.0f)));
					const bool bLampModule = IsLampModule(CellX, CellY, ModuleX, ModuleY);
					const int32 GlobalModuleX = CellX * ModulesPerCell + ModuleX;
					const int32 GlobalModuleY = CellY * ModulesPerCell + ModuleY;
					const bool bVentModule = !bLampModule
						&& Archetype != EBackroomsRoomArchetype::StraightCorridor
						&& Archetype != EBackroomsRoomArchetype::DarkCorridor
						&& FMath::Abs(GlobalModuleX * 3 + GlobalModuleY * 5 + RoomHandle.Value) % 17 == 0;
					const bool bOpenCeilingModule = bLampModule || bVentModule;
					UInstancedStaticMeshComponent* TargetCeilingBatch = bOpenCeilingModule ? LampCeilingBatch : CeilingBatch;
					UStaticMesh* TargetCeilingMesh = bOpenCeilingModule ? LampCeilingMesh.Get() : CeilingMesh.Get();
					TargetCeilingBatch->AddInstance(MakeBoundsCenteredTransform(TargetCeilingMesh, TileLocation + FVector(0.0f, 0.0f, BackroomsCeilingHeight), FRotator::ZeroRotator, FVector(ModuleScale, ModuleScale, 1.0f)));
					if (bLampModule)
					{
						const FVector LampLocation = TileLocation + FVector(0.0f, 0.0f, BackroomsCeilingHeight + BackroomsLampCenterOffset);
						LampBatch->AddInstance(MakeBoundsCenteredTransform(LampMesh, LampLocation));
						AddReferenceLight(LampLocation, LightIntensityScale, RoomLightColor);
					}
					else if (bVentModule)
					{
						const FVector VentLocation = TileLocation + FVector(0.0f, 0.0f, BackroomsCeilingHeight + BackroomsVentCenterOffset);
						VentBatch->AddInstance(MakeBoundsCenteredTransform(VentMesh, VentLocation));
					}
				}
			}

			auto AddWallSide = [this, WallBatch, CellOrigin, ModulePitch, ModuleScale, ModulesPerCell](const bool bAlongX, const float FixedCoordinate)
			{
				for (int32 ModuleIndex = 0; ModuleIndex < ModulesPerCell; ++ModuleIndex)
				{
					const float LongCoordinate = (ModuleIndex + 0.5f) * ModulePitch;
					const FVector Location = CellOrigin + (bAlongX
						? FVector(LongCoordinate, FixedCoordinate, BackroomsCeilingHeight * 0.5f)
						: FVector(FixedCoordinate, LongCoordinate, BackroomsCeilingHeight * 0.5f));
					const FRotator Rotation = bAlongX ? FRotator(0.0f, 90.0f, 0.0f) : FRotator::ZeroRotator;
					WallBatch->AddInstance(MakeBoundsCenteredTransform(WallMesh, Location, Rotation, FVector(1.0f, ModuleScale, 1.0f)));
				}
			};

			if (CellY == Footprint.Y - 1 && !HasConnectorAt(FIntPoint(CellX, CellY), ERoomConnectorDirection::North))
			{
				AddWallSide(true, CellSize);
			}
			if (CellY == 0 && !HasConnectorAt(FIntPoint(CellX, CellY), ERoomConnectorDirection::South))
			{
				AddWallSide(true, 0.0f);
			}
			if (CellX == Footprint.X - 1 && !HasConnectorAt(FIntPoint(CellX, CellY), ERoomConnectorDirection::East))
			{
				AddWallSide(false, CellSize);
			}
			if (CellX == 0 && !HasConnectorAt(FIntPoint(CellX, CellY), ERoomConnectorDirection::West))
			{
				AddWallSide(false, 0.0f);
			}
		}
	}

	auto AddWallRun = [this, WallBatch](const FVector2D& Center, const bool bAlongX, const float Length)
	{
		const int32 ModuleCount = FMath::Max(1, FMath::RoundToInt(Length / BackroomsModuleSize));
		const float Pitch = Length / ModuleCount;
		const float Scale = Pitch / BackroomsModuleSize;
		for (int32 ModuleIndex = 0; ModuleIndex < ModuleCount; ++ModuleIndex)
		{
			const float Offset = -Length * 0.5f + (ModuleIndex + 0.5f) * Pitch;
			const FVector BoundsCenter(
				Center.X + (bAlongX ? Offset : 0.0f),
				Center.Y + (bAlongX ? 0.0f : Offset),
				BackroomsCeilingHeight * 0.5f);
			const FRotator Rotation = bAlongX ? FRotator(0.0f, 90.0f, 0.0f) : FRotator::ZeroRotator;
			WallBatch->AddInstance(MakeBoundsCenteredTransform(WallMesh, BoundsCenter, Rotation, FVector(1.0f, Scale, 1.0f)));
		}
	};

	auto AddPillar = [this, PillarBatch](const FVector2D& Center, const float Scale)
	{
		PillarBatch->AddInstance(MakeBoundsCenteredTransform(
			PillarMesh,
			FVector(Center.X, Center.Y, BackroomsCeilingHeight * 0.5f),
			FRotator::ZeroRotator,
			FVector(Scale, Scale, 1.0f)));
	};

	switch (Archetype)
	{
	case EBackroomsRoomArchetype::Hub:
	case EBackroomsRoomArchetype::OpenPillarHall:
		for (int32 CellX = 0; CellX < Footprint.X; ++CellX)
		{
			for (int32 CellY = 0; CellY < Footprint.Y; ++CellY)
			{
				AddPillar(FVector2D((CellX + 0.5f) * CellSize, (CellY + 0.5f) * CellSize), Archetype == EBackroomsRoomArchetype::Hub ? 1.25f : 1.45f);
			}
		}
		break;
	case EBackroomsRoomArchetype::StraightCorridor:
	case EBackroomsRoomArchetype::DarkCorridor:
		if (bVerticalRoute)
		{
			AddWallRun(FVector2D(CellSize * 0.25f, RoomDepth * 0.5f), false, RoomDepth);
			AddWallRun(FVector2D(CellSize * 0.75f, RoomDepth * 0.5f), false, RoomDepth);
		}
		else
		{
			AddWallRun(FVector2D(RoomWidth * 0.5f, CellSize * 0.25f), true, RoomWidth);
			AddWallRun(FVector2D(RoomWidth * 0.5f, CellSize * 0.75f), true, RoomWidth);
		}
		break;
	case EBackroomsRoomArchetype::CornerCorridor:
	{
		const bool bBlockedLowX = bHasEast;
		const bool bBlockedLowY = bHasNorth;
		AddWallRun(FVector2D(bBlockedLowX ? CellSize * 0.25f : CellSize * 0.75f, CellSize * 0.5f), true, CellSize * 0.5f);
		AddWallRun(FVector2D(CellSize * 0.5f, bBlockedLowY ? CellSize * 0.25f : CellSize * 0.75f), false, CellSize * 0.5f);
		break;
	}
	case EBackroomsRoomArchetype::PartitionedHall:
	{
		const float CentralGap = FMath::Min(600.0f, FMath::Min(RoomWidth, RoomDepth) * 0.25f);
		const float VerticalSegmentLength = FMath::Max(BackroomsModuleSize, (RoomDepth - CentralGap) * 0.5f);
		const float HorizontalSegmentLength = FMath::Max(BackroomsModuleSize, (RoomWidth - CentralGap) * 0.5f);
		AddWallRun(FVector2D(RoomWidth * 0.5f, VerticalSegmentLength * 0.5f), false, VerticalSegmentLength);
		AddWallRun(FVector2D(RoomWidth * 0.5f, RoomDepth - VerticalSegmentLength * 0.5f), false, VerticalSegmentLength);
		AddWallRun(FVector2D(HorizontalSegmentLength * 0.5f, RoomDepth * 0.5f), true, HorizontalSegmentLength);
		AddWallRun(FVector2D(RoomWidth - HorizontalSegmentLength * 0.5f, RoomDepth * 0.5f), true, HorizontalSegmentLength);
		AddPillar(FVector2D(RoomWidth * 0.25f, RoomDepth * 0.25f), 1.25f);
		AddPillar(FVector2D(RoomWidth * 0.75f, RoomDepth * 0.75f), 1.25f);
		break;
	}
	case EBackroomsRoomArchetype::ResourceWorkshop:
	{
		if (RoomDepth >= RoomWidth)
		{
			const float SegmentLength = RoomWidth * 0.25f;
			AddWallRun(FVector2D(SegmentLength * 0.5f, RoomDepth * 0.5f), true, SegmentLength);
			AddWallRun(FVector2D(RoomWidth - SegmentLength * 0.5f, RoomDepth * 0.5f), true, SegmentLength);
		}
		else
		{
			const float SegmentLength = RoomDepth * 0.25f;
			AddWallRun(FVector2D(RoomWidth * 0.5f, SegmentLength * 0.5f), false, SegmentLength);
			AddWallRun(FVector2D(RoomWidth * 0.5f, RoomDepth - SegmentLength * 0.5f), false, SegmentLength);
		}
		for (int32 CellX = 0; CellX < Footprint.X; ++CellX)
		{
			for (int32 CellY = 0; CellY < Footprint.Y; ++CellY)
			{
				AddPillar(FVector2D((CellX + 0.25f) * CellSize, (CellY + 0.5f) * CellSize), 1.10f);
				AddPillar(FVector2D((CellX + 0.75f) * CellSize, (CellY + 0.5f) * CellSize), 1.10f);
			}
		}
		break;
	}
	default:
		break;
	}

	RefreshNavigationRegistration();
}

void ASurvivalRoomRuntimeActor::RefreshNavigationRegistration()
{
	if (!HasAuthority() || !GetWorld())
	{
		return;
	}

	UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavigationSystem)
	{
		return;
	}

	UPrimitiveComponent* const NavigationComponents[] = {
		NavigationFloor,
		WallGeometry,
		PillarGeometry
	};
	for (UPrimitiveComponent* Component : NavigationComponents)
	{
		if (!Component || !Component->IsRegistered() || !Component->CanEverAffectNavigation()
			|| Component->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
		{
			continue;
		}

		Component->UpdateBounds();
		UNavigationSystemV1::UpdateComponentInNavOctree(*Component);
		const FBox DirtyBounds = Component->Bounds.GetBox();
		if (DirtyBounds.IsValid && DirtyBounds.GetVolume() > UE_SMALL_NUMBER)
		{
			NavigationSystem->AddDirtyArea(DirtyBounds, ENavigationDirtyFlag::All, TEXT("SurvivalRoomGeometry"));
		}
	}
}

bool ASurvivalRoomRuntimeActor::HasConnectorAt(const FIntPoint Cell, const ERoomConnectorDirection Direction) const
{
	return RuntimeConnectors.ContainsByPredicate([Cell, Direction](const FRoomConnectorDefinition& Connector)
	{
		return Connector.Cell == Cell && Connector.Direction == Direction;
	});
}

UInstancedStaticMeshComponent* ASurvivalRoomRuntimeActor::ConfigureGeometryBatch(
	UInstancedStaticMeshComponent* Component,
	UStaticMesh* Mesh,
	const ECollisionEnabled::Type CollisionMode,
	const bool bAffectsNavigation)
{
	if (!Component || !Mesh)
	{
		return nullptr;
	}
	Component->SetStaticMesh(Mesh);
	for (int32 MaterialIndex = 0; MaterialIndex < Mesh->GetStaticMaterials().Num(); ++MaterialIndex)
	{
		if (UMaterialInterface* Material = ResolveCompatibleMaterial(Mesh->GetStaticMaterials()[MaterialIndex].MaterialInterface))
		{
			Component->SetMaterial(MaterialIndex, Material);
		}
	}
	Component->SetupAttachment(SceneRoot);
	Component->SetCastShadow(true);
	Component->SetCollisionEnabled(CollisionMode);
	if (CollisionMode != ECollisionEnabled::NoCollision)
	{
		Component->SetCollisionProfileName(TEXT("BlockAll"));
	}
	Component->SetCanEverAffectNavigation(bAffectsNavigation);
	return Component;
}

FTransform ASurvivalRoomRuntimeActor::MakeBoundsCenteredTransform(const UStaticMesh* Mesh, const FVector& BoundsCenter, const FRotator& Rotation, const FVector& Scale) const
{
	const FQuat RotationQuat = Rotation.Quaternion();
	const FVector LocalBoundsOrigin = Mesh ? Mesh->GetBounds().Origin : FVector::ZeroVector;
	const FVector ScaledBoundsOrigin(
		LocalBoundsOrigin.X * Scale.X,
		LocalBoundsOrigin.Y * Scale.Y,
		LocalBoundsOrigin.Z * Scale.Z);
	return FTransform(RotationQuat, BoundsCenter - RotationQuat.RotateVector(ScaledBoundsOrigin), Scale);
}

UMaterialInterface* ASurvivalRoomRuntimeActor::ResolveCompatibleMaterial(UMaterialInterface* SourceMaterial) const
{
	if (!SourceMaterial)
	{
		return nullptr;
	}

	const FName SourceName = SourceMaterial->GetFName();
	if (SourceName == TEXT("carpet"))
	{
		return CarpetMaterial ? CarpetMaterial.Get() : SourceMaterial;
	}
	if (SourceName == TEXT("linoleum"))
	{
		return LinoleumMaterial ? LinoleumMaterial.Get() : SourceMaterial;
	}
	if (SourceName == TEXT("ceiling_panels"))
	{
		return CeilingPanelsMaterial ? CeilingPanelsMaterial.Get() : SourceMaterial;
	}
	if (SourceName == TEXT("ceiling_frame"))
	{
		return CeilingFrameMaterial ? CeilingFrameMaterial.Get() : SourceMaterial;
	}
	if (SourceName == TEXT("lamp"))
	{
		return LampMaterial ? LampMaterial.Get() : SourceMaterial;
	}
	if (SourceName == TEXT("wallpaper"))
	{
		return WallpaperMaterial ? WallpaperMaterial.Get() : SourceMaterial;
	}
	if (SourceName == TEXT("plaster"))
	{
		return PlasterMaterial ? PlasterMaterial.Get() : SourceMaterial;
	}
	if (SourceName == TEXT("plaster_white"))
	{
		return PlasterWhiteMaterial ? PlasterWhiteMaterial.Get() : SourceMaterial;
	}
	if (SourceName == TEXT("wall_trim"))
	{
		return WallTrimMaterial ? WallTrimMaterial.Get() : SourceMaterial;
	}
	if (SourceName == TEXT("wall_trim_dark"))
	{
		return WallTrimDarkMaterial ? WallTrimDarkMaterial.Get() : SourceMaterial;
	}
	if (SourceName == TEXT("ceiling_vent"))
	{
		return CeilingVentMaterial ? CeilingVentMaterial.Get() : SourceMaterial;
	}
	return SourceMaterial;
}

void ASurvivalRoomRuntimeActor::AddReferenceLight(const FVector& RelativeLocation, const float IntensityScale, const FLinearColor& LightColor)
{
	URectLightComponent* Light = NewObject<URectLightComponent>(this);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetupAttachment(SceneRoot);
	Light->SetRelativeLocation(RelativeLocation);
	Light->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	Light->SetIntensity(BackroomsRectLightIntensity * IntensityScale);
	Light->SetLightColor(LightColor);
	Light->SetAttenuationRadius(750.0f);
	Light->SetSourceWidth(96.0f);
	Light->SetSourceHeight(96.0f);
	Light->SetIndirectLightingIntensity(5.0f);
	Light->SetSpecularScale(0.0f);
	Light->SetCastShadows(true);
	Light->RegisterComponent();
	GeneratedLights.Add(Light);
}
