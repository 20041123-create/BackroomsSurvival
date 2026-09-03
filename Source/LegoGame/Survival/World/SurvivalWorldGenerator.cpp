#include "SurvivalWorldGenerator.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "LegoGame/Survival/Contracts/SurvivalGameplayTags.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "SurvivalRoomGateActor.h"
#include "SurvivalRoomRuntimeActor.h"
#include "SurvivalSemanticAnchorActor.h"

namespace
{
	FIntPoint DirectionToCellOffset(const ERoomConnectorDirection Direction)
	{
		switch (Direction)
		{
		case ERoomConnectorDirection::North: return FIntPoint(0, 1);
		case ERoomConnectorDirection::East: return FIntPoint(1, 0);
		case ERoomConnectorDirection::South: return FIntPoint(0, -1);
		case ERoomConnectorDirection::West: return FIntPoint(-1, 0);
		default: return FIntPoint::ZeroValue;
		}
	}
}

ASurvivalWorldGenerator::ASurvivalWorldGenerator()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);
	FallbackRoomActorClass = ASurvivalRoomRuntimeActor::StaticClass();
	GateActorClass = ASurvivalRoomGateActor::StaticClass();
	AnchorActorClass = ASurvivalSemanticAnchorActor::StaticClass();
}

void ASurvivalWorldGenerator::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority() && bGenerateOnBeginPlay)
	{
		GenerateInitialPhase();
	}
}

bool ASurvivalWorldGenerator::RequestGenerateInitialLayout_Implementation(USurvivalModeConfig* Config)
{
	if (!HasAuthority() || LayoutStatus != ESurvivalWorldLayoutStatus::NotRequested || !Config)
	{
		return false;
	}

	ModeConfig = Config;
	LayoutStatus = ESurvivalWorldLayoutStatus::Generating;
	bGenerationSucceeded = false;
	GenerationFailureReason.Reset();
	ForceNetUpdate();

	GenerateInitialLayout();
	return true;
}

bool ASurvivalWorldGenerator::RequestAdvanceToPhase_Implementation(const int32 TargetPhaseIndex)
{
	if (!HasAuthority()
		|| LayoutStatus != ESurvivalWorldLayoutStatus::Succeeded
		|| TargetPhaseIndex <= CurrentPhaseIndex
		|| !IsValidTargetPhase(TargetPhaseIndex))
	{
		return false;
	}

	return MaterializeThroughPhase(TargetPhaseIndex);
}

FSurvivalWorldRuntimeSnapshot ASurvivalWorldGenerator::GetWorldRuntimeSnapshot_Implementation() const
{
	FSurvivalWorldRuntimeSnapshot Snapshot;
	Snapshot.LayoutStatus = LayoutStatus;
	Snapshot.bSucceeded = LayoutStatus == ESurvivalWorldLayoutStatus::Succeeded;
	Snapshot.AppliedSeed = AppliedSeed;
	Snapshot.LayoutHash = LayoutHash;
	Snapshot.CurrentUnlockedPhaseIndex = CurrentPhaseIndex;
	Snapshot.MaterializedRoomCount = MaterializedRoomCount;
	Snapshot.FailureReason = GenerationFailureReason;
	return Snapshot;
}

void ASurvivalWorldGenerator::GetAnchorsByTag_Implementation(const FGameplayTag AnchorTag, TArray<FSurvivalAnchorView>& OutAnchors) const
{
	OutAnchors.Reset();
	if (!AnchorTag.IsValid() || !GetWorld())
	{
		return;
	}

	for (TActorIterator<ASurvivalSemanticAnchorActor> Iterator(GetWorld()); Iterator; ++Iterator)
	{
		const ASurvivalSemanticAnchorActor* Anchor = *Iterator;
		if (!IsValid(Anchor) || Anchor->GetOwner() != this || Anchor->GetAnchorTag() != AnchorTag)
		{
			continue;
		}

		FSurvivalAnchorView& View = OutAnchors.AddDefaulted_GetRef();
		View.AnchorTag = Anchor->GetAnchorTag();
		View.RoomHandle = Anchor->GetOwningRoomHandle();
		View.Transform = Anchor->GetActorTransform();
		View.bEnabled = Anchor->IsAnchorEnabled();
		View.TeamType = Anchor->GetTeamType();
	}

	OutAnchors.Sort([](const FSurvivalAnchorView& Left, const FSurvivalAnchorView& Right)
	{
		if (Left.RoomHandle.Value != Right.RoomHandle.Value)
		{
			return Left.RoomHandle.Value < Right.RoomHandle.Value;
		}
		if (Left.TeamType != Right.TeamType)
		{
			return static_cast<uint8>(Left.TeamType) < static_cast<uint8>(Right.TeamType);
		}

		const FVector LeftLocation = Left.Transform.GetLocation();
		const FVector RightLocation = Right.Transform.GetLocation();
		if (LeftLocation.X != RightLocation.X)
		{
			return LeftLocation.X < RightLocation.X;
		}
		if (LeftLocation.Y != RightLocation.Y)
		{
			return LeftLocation.Y < RightLocation.Y;
		}
		if (LeftLocation.Z != RightLocation.Z)
		{
			return LeftLocation.Z < RightLocation.Z;
		}

		const FRotator LeftRotation = Left.Transform.Rotator();
		const FRotator RightRotation = Right.Transform.Rotator();
		if (LeftRotation.Pitch != RightRotation.Pitch)
		{
			return LeftRotation.Pitch < RightRotation.Pitch;
		}
		if (LeftRotation.Yaw != RightRotation.Yaw)
		{
			return LeftRotation.Yaw < RightRotation.Yaw;
		}
		return LeftRotation.Roll < RightRotation.Roll;
	});
}

bool ASurvivalWorldGenerator::GetTeamPlayerStartTransform_Implementation(const ETeamType TeamType, FTransform& OutTransform) const
{
	if (TeamType != ETeamType::ETT_Police && TeamType != ETeamType::ETT_Bandit)
	{
		return false;
	}

	TArray<FSurvivalAnchorView> PlayerStarts;
	GetAnchorsByTag_Implementation(LG::SurvivalTags::Anchor_PlayerStart, PlayerStarts);
	for (const FSurvivalAnchorView& PlayerStart : PlayerStarts)
	{
		if (PlayerStart.bEnabled && PlayerStart.TeamType == TeamType)
		{
			OutTransform = PlayerStart.Transform;
			return true;
		}
	}
	return false;
}

bool ASurvivalWorldGenerator::GenerateInitialPhase()
{
	if (LayoutStatus == ESurvivalWorldLayoutStatus::NotRequested)
	{
		if (!RequestGenerateInitialLayout_Implementation(ModeConfig))
		{
			return false;
		}
	}
	return LayoutStatus == ESurvivalWorldLayoutStatus::Succeeded;
}

bool ASurvivalWorldGenerator::GenerateInitialLayout()
{
	if (!HasAuthority() || LayoutStatus != ESurvivalWorldLayoutStatus::Generating)
	{
		return false;
	}
	if (!ModeConfig || !StartRoomTemplate)
	{
		SetLayoutFailure(TEXT("Survival world generation needs a mode config and a start room template."));
		return false;
	}

	LG::Survival::World::FLayoutRequest Request;
	Request.Seed = ModeConfig->RandomSeed;
	Request.MaxRoomCount = ModeConfig->MaxRoomCount;
	Request.MaxGenerationAttempts = ModeConfig->MaxGenerationAttempts;
	Request.MinTeamStartGraphDistance = ModeConfig->MinTeamStartGraphDistance;
	Request.StartTemplate = StartRoomTemplate;
	Request.Phases = ModeConfig->Phases;
	for (const TObjectPtr<URoomTemplateData>& Template : RoomTemplates)
	{
		Request.Templates.Add(Template.Get());
	}
	if (!Request.Templates.Contains(StartRoomTemplate))
	{
		Request.Templates.Add(StartRoomTemplate);
	}

	LayoutPlan = LG::Survival::World::FSurvivalLayoutPlanner::Generate(Request);
	AppliedSeed = Request.Seed;
	LayoutHash = static_cast<int32>(LayoutPlan.StableHash);
	if (!LayoutPlan.bSucceeded)
	{
		SetLayoutFailure(LayoutPlan.FailureReason.IsEmpty()
			? FString::Printf(TEXT("Survival layout planning failed for seed %d."), Request.Seed)
			: LayoutPlan.FailureReason);
		return false;
	}
	if (LayoutPlan.Rooms.IsEmpty())
	{
		SetLayoutFailure(TEXT("Survival layout planning returned no rooms."));
		return false;
	}

	UE_LOG(LogTemp, Display,
		TEXT("Survival team starts selected: requested=%d applied=%d fallback=%d PoliceRoom=%d BanditRoom=%d."),
		LayoutPlan.RequestedTeamStartGraphDistance,
		LayoutPlan.AppliedTeamStartGraphDistance,
		LayoutPlan.bUsedFarthestTeamStartFallback,
		LayoutPlan.PoliceTeamStartRoom.Value,
		LayoutPlan.BanditTeamStartRoom.Value);

	int32 InitialPhase = LayoutPlan.Rooms[0].PhaseIndex;
	for (const LG::Survival::World::FPlannedRoom& Room : LayoutPlan.Rooms)
	{
		InitialPhase = FMath::Min(InitialPhase, Room.PhaseIndex);
	}
	if (!MaterializeThroughPhase(InitialPhase))
	{
		SetLayoutFailure(FString::Printf(TEXT("Survival world could not materialize initial phase %d."), InitialPhase));
		return false;
	}

	LayoutStatus = ESurvivalWorldLayoutStatus::Succeeded;
	bGenerationSucceeded = true;
	GenerationFailureReason.Reset();
	ForceNetUpdate();
	return true;
}

bool ASurvivalWorldGenerator::AdvanceToPhase(const int32 TargetPhaseIndex)
{
	return RequestAdvanceToPhase_Implementation(TargetPhaseIndex);
}

void ASurvivalWorldGenerator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASurvivalWorldGenerator, AppliedSeed);
	DOREPLIFETIME(ASurvivalWorldGenerator, LayoutHash);
	DOREPLIFETIME(ASurvivalWorldGenerator, CurrentPhaseIndex);
	DOREPLIFETIME(ASurvivalWorldGenerator, MaterializedRoomCount);
	DOREPLIFETIME(ASurvivalWorldGenerator, bGenerationSucceeded);
	DOREPLIFETIME(ASurvivalWorldGenerator, LayoutStatus);
	DOREPLIFETIME(ASurvivalWorldGenerator, GenerationFailureReason);
}

void ASurvivalWorldGenerator::OnRep_GenerationSummary()
{
	UE_LOG(LogTemp, Log, TEXT("Survival layout replicated: status=%d seed=%d hash=%u phase=%d rooms=%d success=%d failure=%s"),
		static_cast<int32>(LayoutStatus),
		AppliedSeed,
		LayoutHash,
		CurrentPhaseIndex,
		MaterializedRoomCount,
		bGenerationSucceeded,
		*GenerationFailureReason);
}

bool ASurvivalWorldGenerator::MaterializeThroughPhase(const int32 TargetPhaseIndex)
{
	TArray<int32> NewlyMaterializedRoomHandles;
	for (const LG::Survival::World::FPlannedRoom& PlannedRoom : LayoutPlan.Rooms)
	{
		if (PlannedRoom.PhaseIndex <= TargetPhaseIndex && !SpawnedRooms.Contains(PlannedRoom.Handle.Value))
		{
			if (!SpawnPlannedRoom(PlannedRoom))
			{
				UE_LOG(LogTemp, Error, TEXT("Survival world could not spawn planned room %d."), PlannedRoom.Handle.Value);
				RollbackMaterializedRooms(NewlyMaterializedRoomHandles);
				return false;
			}
			NewlyMaterializedRoomHandles.Add(PlannedRoom.Handle.Value);
		}
	}
	CurrentPhaseIndex = TargetPhaseIndex;
	MaterializedRoomCount = SpawnedRooms.Num();
	RefreshGates();
	for (const TPair<int32, TObjectPtr<ASurvivalRoomRuntimeActor>>& Pair : SpawnedRooms)
	{
		if (Pair.Value)
		{
			ISurvivalRoomRuntimeInterface::Execute_SetRoomState(Pair.Value, ESurvivalRoomState::Active);
		}
	}
	RebuildRuntimeNavigation();
	ForceNetUpdate();
	return true;
}

void ASurvivalWorldGenerator::RebuildRuntimeNavigation()
{
	if (!HasAuthority() || !GetWorld())
	{
		return;
	}

	for (const TPair<int32, TObjectPtr<ASurvivalRoomRuntimeActor>>& Pair : SpawnedRooms)
	{
		if (Pair.Value)
		{
			Pair.Value->RefreshNavigationRegistration();
		}
	}

	if (UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		// Layout requests are synchronous. Completing the Recast build here keeps
		// Match from spawning AI before newly materialized rooms are navigable.
		NavigationSystem->Build();

		constexpr float AnchorHeightAboveNavigation = 100.0f;
		const FVector AnchorProjectionExtent(600.0f, 600.0f, 300.0f);
		int32 EnabledAnchorCount = 0;
		int32 NavigableAnchorCount = 0;
		for (TActorIterator<ASurvivalSemanticAnchorActor> Iterator(GetWorld()); Iterator; ++Iterator)
		{
			ASurvivalSemanticAnchorActor* Anchor = *Iterator;
			if (!IsValid(Anchor) || Anchor->GetOwner() != this || !Anchor->IsAnchorEnabled())
			{
				continue;
			}

			++EnabledAnchorCount;
			FNavLocation ProjectedLocation;
			if (!NavigationSystem->ProjectPointToNavigation(
				Anchor->GetActorLocation(), ProjectedLocation, AnchorProjectionExtent))
			{
				Anchor->SetAnchorEnabled(false);
				UE_LOG(LogTemp, Warning,
					TEXT("Survival anchor '%s' (%s, room=%d) has no navigable point within its room and was disabled."),
					*Anchor->GetName(),
					*Anchor->GetAnchorTag().ToString(),
					Anchor->GetOwningRoomHandle().Value);
				continue;
			}

			FVector SafeLocation = ProjectedLocation.Location;
			SafeLocation.Z += AnchorHeightAboveNavigation;
			Anchor->SetActorLocation(SafeLocation, false, nullptr, ETeleportType::TeleportPhysics);
			Anchor->ForceNetUpdate();
			++NavigableAnchorCount;
		}

		UE_LOG(LogTemp, Display,
			TEXT("Survival runtime navigation rebuilt for %d rooms; navigable anchors=%d/%d."),
			SpawnedRooms.Num(), NavigableAnchorCount, EnabledAnchorCount);
	}
}

void ASurvivalWorldGenerator::RollbackMaterializedRooms(const TArray<int32>& RoomHandleValues)
{
	if (RoomHandleValues.IsEmpty())
	{
		return;
	}

	TSet<int32> RoomHandleSet;
	RoomHandleSet.Reserve(RoomHandleValues.Num());
	for (const int32 RoomHandleValue : RoomHandleValues)
	{
		RoomHandleSet.Add(RoomHandleValue);
	}
	for (TActorIterator<ASurvivalSemanticAnchorActor> Iterator(GetWorld()); Iterator; ++Iterator)
	{
		ASurvivalSemanticAnchorActor* Anchor = *Iterator;
		if (IsValid(Anchor)
			&& Anchor->GetOwner() == this
			&& RoomHandleSet.Contains(Anchor->GetOwningRoomHandle().Value))
		{
			Anchor->Destroy();
		}
	}

	for (const int32 RoomHandleValue : RoomHandleValues)
	{
		if (TObjectPtr<ASurvivalRoomRuntimeActor>* Room = SpawnedRooms.Find(RoomHandleValue))
		{
			if (IsValid(Room->Get()))
			{
				Room->Get()->Destroy();
			}
			SpawnedRooms.Remove(RoomHandleValue);
		}
	}
}

bool ASurvivalWorldGenerator::IsValidTargetPhase(const int32 TargetPhaseIndex) const
{
	if (TargetPhaseIndex < 0 || !ModeConfig)
	{
		return false;
	}

	return ModeConfig->Phases.ContainsByPredicate([TargetPhaseIndex](const FSurvivalPhaseDefinition& Phase)
	{
		return Phase.PhaseIndex == TargetPhaseIndex;
	});
}

void ASurvivalWorldGenerator::SetLayoutFailure(const FString& FailureReason)
{
	LayoutStatus = ESurvivalWorldLayoutStatus::Failed;
	bGenerationSucceeded = false;
	GenerationFailureReason = FailureReason;
	UE_LOG(LogTemp, Error, TEXT("%s"), *GenerationFailureReason);
	ForceNetUpdate();
}

bool ASurvivalWorldGenerator::SpawnPlannedRoom(const LG::Survival::World::FPlannedRoom& PlannedRoom)
{
	const TObjectPtr<URoomTemplateData>* TemplatePointer = RoomTemplates.FindByPredicate([&PlannedRoom](const TObjectPtr<URoomTemplateData>& Template)
	{
		return Template && Template->TemplateId == PlannedRoom.TemplateId;
	});
	URoomTemplateData* Template = TemplatePointer ? TemplatePointer->Get() : (StartRoomTemplate && StartRoomTemplate->TemplateId == PlannedRoom.TemplateId ? StartRoomTemplate.Get() : nullptr);
	if (!Template)
	{
		return false;
	}

	TSubclassOf<ASurvivalRoomRuntimeActor> RuntimeClass = FallbackRoomActorClass;
	if (!Template->RoomActorClass.IsNull())
	{
		if (UClass* TemplateClass = Template->RoomActorClass.LoadSynchronous())
		{
			if (TemplateClass->IsChildOf(ASurvivalRoomRuntimeActor::StaticClass()))
			{
				RuntimeClass = TemplateClass;
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Room template '%s' does not reference ASurvivalRoomRuntimeActor."), *Template->TemplateId.ToString());
				return false;
			}
		}
	}
	if (!RuntimeClass)
	{
		return false;
	}

	const FVector Location = FVector(PlannedRoom.Origin.X * GridCellSize, PlannedRoom.Origin.Y * GridCellSize, 0.0f);
	ASurvivalRoomRuntimeActor* Room = GetWorld()->SpawnActorDeferred<ASurvivalRoomRuntimeActor>(RuntimeClass, FTransform(Location), this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Room)
	{
		return false;
	}
	Room->InitializeRoom(PlannedRoom, GridCellSize);
	Room->FinishSpawning(FTransform(Location));
	SpawnedRooms.Add(PlannedRoom.Handle.Value, Room);
	SpawnAnchorsForRoom(*Room, PlannedRoom);
	return true;
}

void ASurvivalWorldGenerator::SpawnAnchorsForRoom(ASurvivalRoomRuntimeActor& Room, const LG::Survival::World::FPlannedRoom& PlannedRoom)
{
	const FVector Center = GetRoomCenter(PlannedRoom);
	if (PlannedRoom.Handle == LayoutPlan.PoliceTeamStartRoom)
	{
		SpawnAnchor(LG::SurvivalTags::Anchor_PlayerStart, Room.GetRoomHandle_Implementation(), Center + FVector(-300.0f, 0.0f, 100.0f), ETeamType::ETT_Police);
		SpawnAnchor(LG::SurvivalTags::Anchor_RespawnBase, Room.GetRoomHandle_Implementation(), Center + FVector(-300.0f, 400.0f, 100.0f), ETeamType::ETT_Police);
	}
	if (PlannedRoom.Handle == LayoutPlan.BanditTeamStartRoom)
	{
		SpawnAnchor(LG::SurvivalTags::Anchor_PlayerStart, Room.GetRoomHandle_Implementation(), Center + FVector(300.0f, 0.0f, 100.0f), ETeamType::ETT_Bandit);
		SpawnAnchor(LG::SurvivalTags::Anchor_RespawnBase, Room.GetRoomHandle_Implementation(), Center + FVector(300.0f, 400.0f, 100.0f), ETeamType::ETT_Bandit);
	}

	if (PlannedRoom.RoomType == LG::SurvivalTags::Room_Type_Monster)
	{
		SpawnAnchor(LG::SurvivalTags::Anchor_Enemy, Room.GetRoomHandle_Implementation(), Center + FVector(-300.0f, -300.0f, 100.0f));
		SpawnAnchor(LG::SurvivalTags::Anchor_Enemy, Room.GetRoomHandle_Implementation(), Center + FVector(300.0f, -300.0f, 100.0f));
		SpawnAnchor(LG::SurvivalTags::Anchor_Enemy, Room.GetRoomHandle_Implementation(), Center + FVector(0.0f, 300.0f, 100.0f));
	}
	else if (PlannedRoom.RoomType == LG::SurvivalTags::Room_Type_HighResource)
	{
		SpawnAnchor(LG::SurvivalTags::Anchor_Resource, Room.GetRoomHandle_Implementation(), Center + FVector(-250.0f, -250.0f, 100.0f));
		SpawnAnchor(LG::SurvivalTags::Anchor_Resource, Room.GetRoomHandle_Implementation(), Center + FVector(250.0f, -250.0f, 100.0f));
		SpawnAnchor(LG::SurvivalTags::Anchor_Resource, Room.GetRoomHandle_Implementation(), Center + FVector(0.0f, 250.0f, 100.0f));
		SpawnAnchor(LG::SurvivalTags::Anchor_Workbench, Room.GetRoomHandle_Implementation(), Center + FVector(0.0f, 0.0f, 100.0f));
	}
	else
	{
		SpawnAnchor(LG::SurvivalTags::Anchor_Resource, Room.GetRoomHandle_Implementation(), Center + FVector(0.0f, 0.0f, 100.0f));
	}
}

void ASurvivalWorldGenerator::SpawnAnchor(const FGameplayTag AnchorTag, const FRoomHandle& RoomHandle, const FVector& Location, const ETeamType TeamType)
{
	if (!AnchorActorClass)
	{
		return;
	}
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ASurvivalSemanticAnchorActor* Anchor = GetWorld()->SpawnActor<ASurvivalSemanticAnchorActor>(AnchorActorClass, Location, FRotator::ZeroRotator, SpawnParameters);
	if (Anchor)
	{
		Anchor->InitializeAnchor(AnchorTag, RoomHandle, TeamType);
	}
}

void ASurvivalWorldGenerator::RefreshGates()
{
	for (const TPair<int32, TObjectPtr<ASurvivalRoomRuntimeActor>>& Pair : SpawnedRooms)
	{
		FRoomHandle RoomHandle;
		RoomHandle.Value = Pair.Key;
		const LG::Survival::World::FPlannedRoom* PlannedRoom = FindPlannedRoom(RoomHandle);
		if (!PlannedRoom || !Pair.Value)
		{
			continue;
		}
		for (const FRoomConnectorDefinition& Connector : Pair.Value->GetRuntimeConnectors())
		{
			const FString GateKey = MakeGateKey(RoomHandle, Connector.ConnectorId);
			const LG::Survival::World::FPlannedConnection* Connection = FindConnection(RoomHandle, Connector.ConnectorId);
			bool bConnectedRoomIsMaterialized = false;
			if (Connection)
			{
				const FRoomHandle OtherRoom = Connection->FirstRoom == RoomHandle ? Connection->SecondRoom : Connection->FirstRoom;
				bConnectedRoomIsMaterialized = SpawnedRooms.Contains(OtherRoom.Value);
			}
			if (Connection && bConnectedRoomIsMaterialized)
			{
				if (TWeakObjectPtr<ASurvivalRoomGateActor>* ExistingGate = SpawnedGates.Find(GateKey))
				{
					if (ExistingGate->IsValid())
					{
						ExistingGate->Get()->UnlockGate();
						ExistingGate->Get()->Destroy();
					}
					SpawnedGates.Remove(GateKey);
				}
			}
			else if (!SpawnedGates.Contains(GateKey))
			{
				SpawnGate(*PlannedRoom, Connector, Connection == nullptr);
			}
		}
	}
}

void ASurvivalWorldGenerator::SpawnGate(const LG::Survival::World::FPlannedRoom& Room, const FRoomConnectorDefinition& Connector, const bool bPermanentSeal)
{
	if (!GateActorClass)
	{
		return;
	}
	ASurvivalRoomGateActor* Gate = GetWorld()->SpawnActor<ASurvivalRoomGateActor>(GateActorClass, GetConnectorWorldLocation(Room, Connector), GetConnectorWorldRotation(Connector), FActorSpawnParameters());
	if (Gate)
	{
		Gate->InitializeGate(Room.Handle, bPermanentSeal, GridCellSize);
		SpawnedGates.Add(MakeGateKey(Room.Handle, Connector.ConnectorId), Gate);
	}
}

const LG::Survival::World::FPlannedConnection* ASurvivalWorldGenerator::FindConnection(const FRoomHandle& RoomHandle, const FName ConnectorId) const
{
	return LayoutPlan.Connections.FindByPredicate([&RoomHandle, ConnectorId](const LG::Survival::World::FPlannedConnection& Connection)
	{
		return (Connection.FirstRoom == RoomHandle && Connection.FirstConnector == ConnectorId)
			|| (Connection.SecondRoom == RoomHandle && Connection.SecondConnector == ConnectorId);
	});
}

const LG::Survival::World::FPlannedRoom* ASurvivalWorldGenerator::FindPlannedRoom(const FRoomHandle& RoomHandle) const
{
	return LayoutPlan.Rooms.FindByPredicate([&RoomHandle](const LG::Survival::World::FPlannedRoom& Room)
	{
		return Room.Handle == RoomHandle;
	});
}

FString ASurvivalWorldGenerator::MakeGateKey(const FRoomHandle& RoomHandle, const FName ConnectorId) const
{
	return FString::Printf(TEXT("%d:%s"), RoomHandle.Value, *ConnectorId.ToString());
}

FVector ASurvivalWorldGenerator::GetConnectorWorldLocation(const LG::Survival::World::FPlannedRoom& Room, const FRoomConnectorDefinition& Connector) const
{
	const FIntPoint DirectionOffset = DirectionToCellOffset(Connector.Direction);
	const FVector CellCenter = FVector((Room.Origin.X + Connector.Cell.X + 0.5f) * GridCellSize, (Room.Origin.Y + Connector.Cell.Y + 0.5f) * GridCellSize, 150.0f);
	return CellCenter + FVector(DirectionOffset.X * GridCellSize * 0.5f, DirectionOffset.Y * GridCellSize * 0.5f, 0.0f);
}

FRotator ASurvivalWorldGenerator::GetConnectorWorldRotation(const FRoomConnectorDefinition& Connector) const
{
	return Connector.Direction == ERoomConnectorDirection::North || Connector.Direction == ERoomConnectorDirection::South
		? FRotator(0.0f, 90.0f, 0.0f)
		: FRotator::ZeroRotator;
}

FVector ASurvivalWorldGenerator::GetRoomCenter(const LG::Survival::World::FPlannedRoom& Room) const
{
	return FVector((Room.Origin.X + Room.Footprint.X * 0.5f) * GridCellSize, (Room.Origin.Y + Room.Footprint.Y * 0.5f) * GridCellSize, 0.0f);
}
