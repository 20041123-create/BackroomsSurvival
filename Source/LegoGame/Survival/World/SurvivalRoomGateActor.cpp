#include "SurvivalRoomGateActor.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float GateBackroomsModuleSize = 300.0f;
	constexpr float GateBackroomsWallCenterHeight = 150.0f;
}

ASurvivalRoomGateActor::ASurvivalRoomGateActor()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	GateMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GateMesh"));
	GateMesh->SetupAttachment(RootComponent);
	GateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GateMesh->SetCanEverAffectNavigation(false);

	GateBlocker = CreateDefaultSubobject<UBoxComponent>(TEXT("GateBlocker"));
	GateBlocker->SetupAttachment(RootComponent);
	GateBlocker->SetCollisionProfileName(TEXT("BlockAll"));
	GateBlocker->SetCanEverAffectNavigation(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> WallAsset(TEXT("/Game/BackRooms/backrooms1_wall_straight.backrooms1_wall_straight"));
	if (WallAsset.Succeeded())
	{
		WallMesh = WallAsset.Object;
		GateMesh->SetStaticMesh(WallMesh);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WallpaperAsset(TEXT("/Game/LegoGame/Survival/Materials/M_Survival_Backrooms_Wallpaper.M_Survival_Backrooms_Wallpaper"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlasterAsset(TEXT("/Game/LegoGame/Survival/Materials/M_Survival_Backrooms_Plaster.M_Survival_Backrooms_Plaster"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WallTrimAsset(TEXT("/Game/LegoGame/Survival/Materials/M_Survival_Backrooms_WallTrim.M_Survival_Backrooms_WallTrim"));
	if (WallpaperAsset.Succeeded())
	{
		GateMesh->SetMaterial(0, WallpaperAsset.Object);
	}
	if (PlasterAsset.Succeeded())
	{
		GateMesh->SetMaterial(1, PlasterAsset.Object);
	}
	if (WallTrimAsset.Succeeded())
	{
		GateMesh->SetMaterial(2, WallTrimAsset.Object);
	}
	RebuildGateGeometry();
}

void ASurvivalRoomGateActor::InitializeGate(const FRoomHandle InOwningRoom, const bool bInPermanentSeal, const float InOpeningWidth)
{
	if (!HasAuthority())
	{
		return;
	}
	OwningRoomHandle = InOwningRoom;
	bPermanentSeal = bInPermanentSeal;
	OpeningWidth = FMath::Max(GateBackroomsModuleSize, InOpeningWidth);
	bLocked = true;
	RebuildGateGeometry();
	ApplyGateState();
	ForceNetUpdate();
}

void ASurvivalRoomGateActor::UnlockGate()
{
	if (!HasAuthority() || bPermanentSeal || !bLocked)
	{
		return;
	}
	bLocked = false;
	ApplyGateState();
	ForceNetUpdate();
}

void ASurvivalRoomGateActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASurvivalRoomGateActor, bLocked);
	DOREPLIFETIME(ASurvivalRoomGateActor, bPermanentSeal);
	DOREPLIFETIME(ASurvivalRoomGateActor, OwningRoomHandle);
	DOREPLIFETIME(ASurvivalRoomGateActor, OpeningWidth);
}

void ASurvivalRoomGateActor::OnRep_GateState()
{
	ApplyGateState();
}

void ASurvivalRoomGateActor::OnRep_GateLayout()
{
	RebuildGateGeometry();
}

void ASurvivalRoomGateActor::RebuildGateGeometry()
{
	GateMesh->ClearInstances();
	if (!WallMesh)
	{
		return;
	}

	const int32 ModuleCount = FMath::Max(1, FMath::RoundToInt(OpeningWidth / GateBackroomsModuleSize));
	const float ModulePitch = OpeningWidth / ModuleCount;
	const float ModuleScale = ModulePitch / GateBackroomsModuleSize;
	const FVector BoundsOrigin = WallMesh->GetBounds().Origin;
	const FVector ScaledBoundsOrigin(BoundsOrigin.X, BoundsOrigin.Y * ModuleScale, BoundsOrigin.Z);
	for (int32 ModuleIndex = 0; ModuleIndex < ModuleCount; ++ModuleIndex)
	{
		const FVector BoundsCenter(0.0f, -OpeningWidth * 0.5f + (ModuleIndex + 0.5f) * ModulePitch, 0.0f);
		GateMesh->AddInstance(FTransform(FRotator::ZeroRotator, BoundsCenter - ScaledBoundsOrigin, FVector(1.0f, ModuleScale, 1.0f)));
	}
	GateBlocker->SetBoxExtent(FVector(20.0f, OpeningWidth * 0.5f, GateBackroomsWallCenterHeight));
}

void ASurvivalRoomGateActor::ApplyGateState()
{
	GateMesh->SetVisibility(bLocked, true);
	GateBlocker->SetCollisionEnabled(bLocked ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	GateBlocker->SetCanEverAffectNavigation(bLocked);
	UNavigationSystemV1::UpdateComponentInNavOctree(*GateBlocker);
}
