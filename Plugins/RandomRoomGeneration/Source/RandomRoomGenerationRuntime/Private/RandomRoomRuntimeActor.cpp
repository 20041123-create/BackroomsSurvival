#include "RandomRoomRuntimeActor.h"

#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ARandomRoomRuntimeActor::ARandomRoomRuntimeActor()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(true);
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded()) { BlockMesh = CubeMesh.Object; }
}

void ARandomRoomRuntimeActor::InitializeRoom(const FRandomRoomPlannedRoom& InPlan, const float InCellSize)
{
	if (!HasAuthority()) { return; }
	RoomHandle = InPlan.Handle;
	TemplateId = InPlan.TemplateId;
	RoomTypeTag = InPlan.RoomType;
	Footprint = InPlan.Footprint;
	RotationQuarterTurns = InPlan.RotationQuarterTurns;
	UnlockPhaseIndex = InPlan.PhaseIndex;
	CellSize = InCellSize;
	RuntimeConnectors.Reset(InPlan.Connectors.Num());
	for (const FRandomRoomPlannedConnector& Connector : InPlan.Connectors)
	{
		FRandomRoomConnectorDefinition& RuntimeConnector = RuntimeConnectors.AddDefaulted_GetRef();
		RuntimeConnector.ConnectorId = Connector.ConnectorId;
		RuntimeConnector.Cell = Connector.Cell;
		RuntimeConnector.Direction = Connector.Direction;
		RuntimeConnector.ConnectorTags = Connector.Tags;
	}
	RoomState = ERandomRoomState::Locked;
	RebuildGeometry();
	ForceNetUpdate();
}

void ARandomRoomRuntimeActor::SetRoomState(const ERandomRoomState NewState)
{
	if (HasAuthority() && RoomState != NewState) { RoomState = NewState; ForceNetUpdate(); }
}

void ARandomRoomRuntimeActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARandomRoomRuntimeActor, RoomHandle);
	DOREPLIFETIME(ARandomRoomRuntimeActor, TemplateId);
	DOREPLIFETIME(ARandomRoomRuntimeActor, RoomTypeTag);
	DOREPLIFETIME(ARandomRoomRuntimeActor, Footprint);
	DOREPLIFETIME(ARandomRoomRuntimeActor, RuntimeConnectors);
	DOREPLIFETIME(ARandomRoomRuntimeActor, RotationQuarterTurns);
	DOREPLIFETIME(ARandomRoomRuntimeActor, UnlockPhaseIndex);
	DOREPLIFETIME(ARandomRoomRuntimeActor, CellSize);
	DOREPLIFETIME(ARandomRoomRuntimeActor, RoomState);
}

void ARandomRoomRuntimeActor::OnRep_RoomLayout() { RebuildGeometry(); }
void ARandomRoomRuntimeActor::OnRep_RoomState() {}

void ARandomRoomRuntimeActor::RebuildGeometry()
{
	for (UStaticMeshComponent* Component : GeneratedGeometry) { if (Component) { Component->DestroyComponent(); } }
	GeneratedGeometry.Reset();
	if (!BlockMesh || Footprint.X <= 0 || Footprint.Y <= 0 || CellSize <= 0.0f) { return; }
	const float WallThickness = 30.0f;
	const float WallHeight = 500.0f;
	AddWall(FVector(Footprint.X * CellSize * 0.5f, Footprint.Y * CellSize * 0.5f, -25.0f), FVector(Footprint.X * CellSize / 100.0f, Footprint.Y * CellSize / 100.0f, 0.25f));
	for (int32 X = 0; X < Footprint.X; ++X)
	{
		if (!HasConnectorAt(FIntPoint(X, Footprint.Y - 1), ERandomRoomConnectorDirection::North)) { AddWall(FVector((X + .5f) * CellSize, Footprint.Y * CellSize, WallHeight), FVector(CellSize / 100.0f, WallThickness / 100.0f, WallHeight / 100.0f)); }
		if (!HasConnectorAt(FIntPoint(X, 0), ERandomRoomConnectorDirection::South)) { AddWall(FVector((X + .5f) * CellSize, 0.f, WallHeight), FVector(CellSize / 100.0f, WallThickness / 100.0f, WallHeight / 100.0f)); }
	}
	for (int32 Y = 0; Y < Footprint.Y; ++Y)
	{
		if (!HasConnectorAt(FIntPoint(Footprint.X - 1, Y), ERandomRoomConnectorDirection::East)) { AddWall(FVector(Footprint.X * CellSize, (Y + .5f) * CellSize, WallHeight), FVector(WallThickness / 100.0f, CellSize / 100.0f, WallHeight / 100.0f)); }
		if (!HasConnectorAt(FIntPoint(0, Y), ERandomRoomConnectorDirection::West)) { AddWall(FVector(0.f, (Y + .5f) * CellSize, WallHeight), FVector(WallThickness / 100.0f, CellSize / 100.0f, WallHeight / 100.0f)); }
	}
}

bool ARandomRoomRuntimeActor::HasConnectorAt(const FIntPoint Cell, const ERandomRoomConnectorDirection Direction) const
{
	return RuntimeConnectors.ContainsByPredicate([Cell, Direction](const FRandomRoomConnectorDefinition& Connector) { return Connector.Cell == Cell && Connector.Direction == Direction; });
}

void ARandomRoomRuntimeActor::AddWall(const FVector& RelativeLocation, const FVector& RelativeScale)
{
	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this);
	Component->SetStaticMesh(BlockMesh);
	Component->SetupAttachment(SceneRoot);
	Component->SetRelativeLocation(RelativeLocation);
	Component->SetRelativeScale3D(RelativeScale);
	Component->SetCollisionProfileName(TEXT("BlockAll"));
	Component->SetCanEverAffectNavigation(true);
	Component->RegisterComponent();
	GeneratedGeometry.Add(Component);
}
