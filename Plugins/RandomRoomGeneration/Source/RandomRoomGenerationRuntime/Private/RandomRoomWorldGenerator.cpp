#include "RandomRoomWorldGenerator.h"

#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "RandomRoomGateActor.h"
#include "RandomRoomLayoutPlanner.h"
#include "RandomRoomRuntimeActor.h"
#include "RandomRoomSemanticAnchorActor.h"

namespace
{
	FIntPoint DirectionToCellOffset(const ERandomRoomConnectorDirection Direction)
	{
		switch (Direction)
		{
		case ERandomRoomConnectorDirection::North: return FIntPoint(0, 1);
		case ERandomRoomConnectorDirection::East: return FIntPoint(1, 0);
		case ERandomRoomConnectorDirection::South: return FIntPoint(0, -1);
		case ERandomRoomConnectorDirection::West: return FIntPoint(-1, 0);
		default: return FIntPoint::ZeroValue;
		}
	}
}

ARandomRoomWorldGenerator::ARandomRoomWorldGenerator()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);
	RoomActorClass = ARandomRoomRuntimeActor::StaticClass();
	GateActorClass = ARandomRoomGateActor::StaticClass();
	AnchorActorClass = ARandomRoomSemanticAnchorActor::StaticClass();
}

bool ARandomRoomWorldGenerator::InitializeLayout(const FRandomRoomLayoutPlan& InPlan, const float InGridCellSize)
{
	if (!HasAuthority() || bGenerationSucceeded || !InPlan.bSucceeded || InPlan.Rooms.IsEmpty() || InGridCellSize <= 0.f) { return false; }
	FString FailureReason;
	if (!RandomRoomGeneration::FRandomRoomLayoutPlanner::VerifyPlan(InPlan, FailureReason)) { return false; }
	LayoutPlan = InPlan;
	GridCellSize = InGridCellSize;
	AppliedSeed = InPlan.Seed;
	LayoutHash = static_cast<int32>(InPlan.StableHash);
	int32 InitialPhase = InPlan.Rooms[0].PhaseIndex;
	for (const FRandomRoomPlannedRoom& Room : InPlan.Rooms) { InitialPhase = FMath::Min(InitialPhase, Room.PhaseIndex); }
	return MaterializeThroughPhase(InitialPhase);
}

bool ARandomRoomWorldGenerator::AdvanceToPhase(const int32 TargetPhaseIndex)
{
	return HasAuthority() && bGenerationSucceeded && TargetPhaseIndex > CurrentPhaseIndex && MaterializeThroughPhase(TargetPhaseIndex);
}

ARandomRoomSemanticAnchorActor* ARandomRoomWorldGenerator::SpawnSemanticAnchor(const FGameplayTag AnchorTag, const FRandomRoomHandle RoomHandle, const FVector Location, const FGameplayTagContainer ContextTags)
{
	if (!HasAuthority() || !AnchorActorClass) { return nullptr; }
	ARandomRoomSemanticAnchorActor* Anchor = GetWorld()->SpawnActor<ARandomRoomSemanticAnchorActor>(AnchorActorClass, Location, FRotator::ZeroRotator);
	if (Anchor) { Anchor->InitializeAnchor(AnchorTag, RoomHandle, ContextTags); }
	return Anchor;
}

void ARandomRoomWorldGenerator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARandomRoomWorldGenerator, AppliedSeed);
	DOREPLIFETIME(ARandomRoomWorldGenerator, LayoutHash);
	DOREPLIFETIME(ARandomRoomWorldGenerator, CurrentPhaseIndex);
	DOREPLIFETIME(ARandomRoomWorldGenerator, MaterializedRoomCount);
	DOREPLIFETIME(ARandomRoomWorldGenerator, bGenerationSucceeded);
}

void ARandomRoomWorldGenerator::OnRep_GenerationSummary()
{
	UE_LOG(LogTemp, Log, TEXT("Random room layout replicated: seed=%d hash=%u phase=%d rooms=%d success=%d"), AppliedSeed, LayoutHash, CurrentPhaseIndex, MaterializedRoomCount, bGenerationSucceeded);
}

void ARandomRoomWorldGenerator::ConfigureRoom(ARandomRoomRuntimeActor& Room, const FRandomRoomPlannedRoom& PlannedRoom) { Room.InitializeRoom(PlannedRoom, GridCellSize); }
void ARandomRoomWorldGenerator::ConfigureRoomAnchors(ARandomRoomRuntimeActor& Room, const FRandomRoomPlannedRoom& PlannedRoom) {}

bool ARandomRoomWorldGenerator::MaterializeThroughPhase(const int32 TargetPhaseIndex)
{
	bool bSpawnedAnyRooms = false;
	for (const FRandomRoomPlannedRoom& PlannedRoom : LayoutPlan.Rooms)
	{
		if (PlannedRoom.PhaseIndex <= TargetPhaseIndex && !SpawnedRooms.Contains(PlannedRoom.Handle.Value))
		{
			if (!SpawnPlannedRoom(PlannedRoom)) { return false; }
			bSpawnedAnyRooms = true;
		}
	}
	if (!bSpawnedAnyRooms && CurrentPhaseIndex != INDEX_NONE) { return false; }
	CurrentPhaseIndex = TargetPhaseIndex;
	MaterializedRoomCount = SpawnedRooms.Num();
	bGenerationSucceeded = true;
	RefreshGates();
	for (const TPair<int32, TObjectPtr<ARandomRoomRuntimeActor>>& Pair : SpawnedRooms) { if (Pair.Value) { Pair.Value->SetRoomState(ERandomRoomState::Active); } }
	ForceNetUpdate();
	return true;
}

bool ARandomRoomWorldGenerator::SpawnPlannedRoom(const FRandomRoomPlannedRoom& PlannedRoom)
{
	if (!RoomActorClass) { return false; }
	const FVector Location(PlannedRoom.Origin.X * GridCellSize, PlannedRoom.Origin.Y * GridCellSize, 0.f);
	ARandomRoomRuntimeActor* Room = GetWorld()->SpawnActorDeferred<ARandomRoomRuntimeActor>(RoomActorClass, FTransform(Location), this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Room) { return false; }
	ConfigureRoom(*Room, PlannedRoom);
	Room->FinishSpawning(FTransform(Location));
	SpawnedRooms.Add(PlannedRoom.Handle.Value, Room);
	ConfigureRoomAnchors(*Room, PlannedRoom);
	ReceiveRoomMaterialized(Room, PlannedRoom.RoomType);
	return true;
}

void ARandomRoomWorldGenerator::RefreshGates()
{
	for (const TPair<int32, TObjectPtr<ARandomRoomRuntimeActor>>& Pair : SpawnedRooms)
	{
		FRandomRoomHandle RoomHandle; RoomHandle.Value = Pair.Key;
		const FRandomRoomPlannedRoom* PlannedRoom = FindPlannedRoom(RoomHandle);
		if (!PlannedRoom || !Pair.Value) { continue; }
		for (const FRandomRoomConnectorDefinition& Connector : Pair.Value->GetRuntimeConnectors())
		{
			const FString GateKey = MakeGateKey(RoomHandle, Connector.ConnectorId);
			const FRandomRoomPlannedConnection* Connection = FindConnection(RoomHandle, Connector.ConnectorId);
			bool bConnectedRoomIsMaterialized = false;
			if (Connection)
			{
				const FRandomRoomHandle OtherRoom = Connection->FirstRoom == RoomHandle ? Connection->SecondRoom : Connection->FirstRoom;
				bConnectedRoomIsMaterialized = SpawnedRooms.Contains(OtherRoom.Value);
			}
			if (Connection && bConnectedRoomIsMaterialized)
			{
				if (TWeakObjectPtr<ARandomRoomGateActor>* ExistingGate = SpawnedGates.Find(GateKey)) { if (ExistingGate->IsValid()) { ExistingGate->Get()->UnlockGate(); ExistingGate->Get()->Destroy(); } SpawnedGates.Remove(GateKey); }
			}
			else if (!SpawnedGates.Contains(GateKey)) { SpawnGate(*PlannedRoom, Connector, Connection == nullptr); }
		}
	}
}

void ARandomRoomWorldGenerator::SpawnGate(const FRandomRoomPlannedRoom& Room, const FRandomRoomConnectorDefinition& Connector, const bool bPermanentSeal)
{
	if (!GateActorClass) { return; }
	ARandomRoomGateActor* Gate = GetWorld()->SpawnActor<ARandomRoomGateActor>(GateActorClass, GetConnectorWorldLocation(Room, Connector), GetConnectorWorldRotation(Connector));
	if (Gate) { Gate->InitializeGate(Room.Handle, bPermanentSeal); SpawnedGates.Add(MakeGateKey(Room.Handle, Connector.ConnectorId), Gate); }
}

const FRandomRoomPlannedConnection* ARandomRoomWorldGenerator::FindConnection(const FRandomRoomHandle RoomHandle, const FName ConnectorId) const
{
	return LayoutPlan.Connections.FindByPredicate([RoomHandle, ConnectorId](const FRandomRoomPlannedConnection& Connection) { return (Connection.FirstRoom == RoomHandle && Connection.FirstConnector == ConnectorId) || (Connection.SecondRoom == RoomHandle && Connection.SecondConnector == ConnectorId); });
}

const FRandomRoomPlannedRoom* ARandomRoomWorldGenerator::FindPlannedRoom(const FRandomRoomHandle RoomHandle) const { return LayoutPlan.Rooms.FindByPredicate([RoomHandle](const FRandomRoomPlannedRoom& Room) { return Room.Handle == RoomHandle; }); }
FString ARandomRoomWorldGenerator::MakeGateKey(const FRandomRoomHandle RoomHandle, const FName ConnectorId) const { return FString::Printf(TEXT("%d:%s"), RoomHandle.Value, *ConnectorId.ToString()); }

FVector ARandomRoomWorldGenerator::GetConnectorWorldLocation(const FRandomRoomPlannedRoom& Room, const FRandomRoomConnectorDefinition& Connector) const
{
	const FIntPoint DirectionOffset = DirectionToCellOffset(Connector.Direction);
	const FVector CellCenter((Room.Origin.X + Connector.Cell.X + .5f) * GridCellSize, (Room.Origin.Y + Connector.Cell.Y + .5f) * GridCellSize, 250.f);
	return CellCenter + FVector(DirectionOffset.X * GridCellSize * .5f, DirectionOffset.Y * GridCellSize * .5f, 0.f);
}

FRotator ARandomRoomWorldGenerator::GetConnectorWorldRotation(const FRandomRoomConnectorDefinition& Connector) const
{
	return Connector.Direction == ERandomRoomConnectorDirection::North || Connector.Direction == ERandomRoomConnectorDirection::South ? FRotator(0.f, 90.f, 0.f) : FRotator::ZeroRotator;
}
