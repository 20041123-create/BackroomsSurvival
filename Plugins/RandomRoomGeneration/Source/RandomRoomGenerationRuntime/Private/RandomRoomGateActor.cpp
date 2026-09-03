#include "RandomRoomGateActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ARandomRoomGateActor::ARandomRoomGateActor()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	GateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GateMesh"));
	GateMesh->SetupAttachment(RootComponent);
	GateMesh->SetRelativeScale3D(FVector(.15f, 10.f, 2.5f));
	GateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GateBlocker = CreateDefaultSubobject<UBoxComponent>(TEXT("GateBlocker"));
	GateBlocker->SetupAttachment(RootComponent);
	GateBlocker->SetBoxExtent(FVector(15.f, 1000.f, 250.f));
	GateBlocker->SetCollisionProfileName(TEXT("BlockAll"));
	GateBlocker->SetCanEverAffectNavigation(true);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded()) { GateMesh->SetStaticMesh(CubeMesh.Object); }
}

void ARandomRoomGateActor::InitializeGate(const FRandomRoomHandle InOwningRoom, const bool bInPermanentSeal)
{
	if (!HasAuthority()) { return; }
	OwningRoomHandle = InOwningRoom;
	bPermanentSeal = bInPermanentSeal;
	bLocked = true;
	ApplyGateState();
	ForceNetUpdate();
}

void ARandomRoomGateActor::UnlockGate()
{
	if (HasAuthority() && !bPermanentSeal && bLocked) { bLocked = false; ApplyGateState(); ForceNetUpdate(); }
}

void ARandomRoomGateActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARandomRoomGateActor, bLocked);
	DOREPLIFETIME(ARandomRoomGateActor, bPermanentSeal);
	DOREPLIFETIME(ARandomRoomGateActor, OwningRoomHandle);
}

void ARandomRoomGateActor::OnRep_GateState() { ApplyGateState(); }
void ARandomRoomGateActor::ApplyGateState()
{
	GateMesh->SetVisibility(bLocked, true);
	GateBlocker->SetCollisionEnabled(bLocked ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	GateBlocker->SetCanEverAffectNavigation(bLocked);
	UNavigationSystemV1::UpdateComponentInNavOctree(*GateBlocker);
}
