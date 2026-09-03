#include "RandomRoomLayoutPlanner.h"


#include "Misc/Crc.h"

namespace RandomRoomGeneration
{
	namespace
	{
		struct FFrontierConnector
		{
			int32 RoomIndex = INDEX_NONE;
			int32 ConnectorIndex = INDEX_NONE;
		};

		struct FPlacementCandidate
		{
			int32 FrontierIndex = INDEX_NONE;
			const FRandomRoomTemplateDefinition* Template = nullptr;
			FGameplayTag RoomType;
			int32 RotationQuarterTurns = 0;
			int32 ConnectorIndex = INDEX_NONE;
			FIntPoint Origin = FIntPoint::ZeroValue;
			float Weight = 0.0f;
		};

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

		ERandomRoomConnectorDirection OppositeDirection(const ERandomRoomConnectorDirection Direction)
		{
			return static_cast<ERandomRoomConnectorDirection>((static_cast<uint8>(Direction) + 2) % 4);
		}

		ERandomRoomConnectorDirection RotateDirection(const ERandomRoomConnectorDirection Direction, const int32 RotationQuarterTurns)
		{
			return static_cast<ERandomRoomConnectorDirection>((static_cast<uint8>(Direction) + RotationQuarterTurns) % 4);
		}

		FIntPoint GetRotatedFootprint(const FIntPoint Footprint, const int32 RotationQuarterTurns)
		{
			return RotationQuarterTurns % 2 == 0 ? Footprint : FIntPoint(Footprint.Y, Footprint.X);
		}

		FIntPoint RotateCell(const FIntPoint Cell, const FIntPoint Footprint, const int32 RotationQuarterTurns)
		{
			switch (RotationQuarterTurns)
			{
			case 0: return Cell;
			case 1: return FIntPoint(Footprint.Y - 1 - Cell.Y, Cell.X);
			case 2: return FIntPoint(Footprint.X - 1 - Cell.X, Footprint.Y - 1 - Cell.Y);
			case 3: return FIntPoint(Cell.Y, Footprint.X - 1 - Cell.X);
			default: return Cell;
			}
		}

		bool AreConnectorTagsCompatible(const FGameplayTagContainer& First, const FGameplayTagContainer& Second)
		{
			return First.IsEmpty() || Second.IsEmpty() || First.HasAny(Second);
		}

		bool IsCellInsideFootprint(const FIntPoint Cell, const FIntPoint Footprint)
		{
			return Cell.X >= 0 && Cell.Y >= 0 && Cell.X < Footprint.X && Cell.Y < Footprint.Y;
		}

		bool IsConnectorOnMatchingBoundary(const FRandomRoomConnectorDefinition& Connector, const FIntPoint Footprint)
		{
			switch (Connector.Direction)
			{
			case ERandomRoomConnectorDirection::North: return Connector.Cell.Y == Footprint.Y - 1;
			case ERandomRoomConnectorDirection::East: return Connector.Cell.X == Footprint.X - 1;
			case ERandomRoomConnectorDirection::South: return Connector.Cell.Y == 0;
			case ERandomRoomConnectorDirection::West: return Connector.Cell.X == 0;
			default: return false;
			}
		}

		FRandomRoomPlannedConnector MakeRotatedConnector(const FRandomRoomConnectorDefinition& Connector, const FIntPoint Footprint, const int32 RotationQuarterTurns)
		{
			FRandomRoomPlannedConnector Result;
			Result.ConnectorId = Connector.ConnectorId;
			Result.Cell = RotateCell(Connector.Cell, Footprint, RotationQuarterTurns);
			Result.Direction = RotateDirection(Connector.Direction, RotationQuarterTurns);
			Result.Tags = Connector.ConnectorTags;
			return Result;
		}

		bool DoesFootprintOverlap(const FIntPoint Origin, const FIntPoint Footprint, const TSet<FIntPoint>& OccupiedCells)
		{
			for (int32 X = 0; X < Footprint.X; ++X)
			{
				for (int32 Y = 0; Y < Footprint.Y; ++Y)
				{
					if (OccupiedCells.Contains(Origin + FIntPoint(X, Y)))
					{
						return true;
					}
				}
			}
			return false;
		}

		void AddFootprintToOccupiedCells(const FRandomRoomPlannedRoom& Room, TSet<FIntPoint>& OccupiedCells)
		{
			for (int32 X = 0; X < Room.Footprint.X; ++X)
			{
				for (int32 Y = 0; Y < Room.Footprint.Y; ++Y)
				{
					OccupiedCells.Add(Room.Origin + FIntPoint(X, Y));
				}
			}
		}

		float GetRoomTypeWeight(const FRandomRoomPhaseDefinition& Phase, const FGameplayTag RoomType)
		{
			if (Phase.RoomTypeWeights.IsEmpty())
			{
				return 1.0f;
			}
			if (const float* Weight = Phase.RoomTypeWeights.Find(RoomType))
			{
				return FMath::Max(0.0f, *Weight);
			}
			return 0.0f;
		}

		TArray<FRandomRoomPhaseDefinition> GetSortedPhases(const TArray<FRandomRoomPhaseDefinition>& Phases)
		{
			TArray<FRandomRoomPhaseDefinition> Result = Phases;
			Result.Sort([](const FRandomRoomPhaseDefinition& Left, const FRandomRoomPhaseDefinition& Right)
			{
				return Left.PhaseIndex < Right.PhaseIndex;
			});
			return Result;
		}

		TArray<int32> BuildPhaseSlots(const FRandomRoomGenerationRequest& Request)
		{
			const TArray<FRandomRoomPhaseDefinition> SortedPhases = GetSortedPhases(Request.Phases);
			TArray<int32> Result;
			for (const FRandomRoomPhaseDefinition& Phase : SortedPhases)
			{
				for (int32 RoomIndex = 0; RoomIndex < Phase.RoomsToUnlock && Result.Num() < Request.MaxRoomCount; ++RoomIndex)
				{
					Result.Add(Phase.PhaseIndex);
				}
			}
			if (Result.IsEmpty())
			{
				Result.Init(0, Request.MaxRoomCount);
			}
			return Result;
		}

		const FRandomRoomPhaseDefinition* FindPhaseByIndex(const TArray<FRandomRoomPhaseDefinition>& Phases, const int32 PhaseIndex)
		{
			for (const FRandomRoomPhaseDefinition& Phase : Phases)
			{
				if (Phase.PhaseIndex == PhaseIndex)
				{
					return &Phase;
				}
			}
			return nullptr;
		}

		FGameplayTag ChooseStartRoomType(const FRandomRoomTemplateDefinition& StartTemplate)
		{
			TArray<FGameplayTag> Tags;
			StartTemplate.AllowedRoomTypes.GetGameplayTagArray(Tags);
			Tags.Sort([](const FGameplayTag& Left, const FGameplayTag& Right)
			{
				return Left.ToString() < Right.ToString();
			});
			return Tags.IsEmpty() ? FGameplayTag() : Tags[0];
		}

		uint32 AddNameToHash(uint32 CurrentHash, const FName Name)
		{
			return FCrc::StrCrc32(*Name.ToString(), CurrentHash);
		}

		uint32 AddTagToHash(uint32 CurrentHash, const FGameplayTag Tag)
		{
			return FCrc::StrCrc32(*Tag.ToString(), CurrentHash);
		}

		uint32 CalculateStableHash(const FRandomRoomLayoutPlan& Plan)
		{
			uint32 Hash = 0;
			Hash = FCrc::MemCrc32(&Plan.Seed, sizeof(Plan.Seed), Hash);
			for (const FRandomRoomPlannedRoom& Room : Plan.Rooms)
			{
				Hash = FCrc::MemCrc32(&Room.Handle.Value, sizeof(Room.Handle.Value), Hash);
				Hash = AddNameToHash(Hash, Room.TemplateId);
				Hash = AddTagToHash(Hash, Room.RoomType);
				Hash = FCrc::MemCrc32(&Room.Origin, sizeof(Room.Origin), Hash);
				Hash = FCrc::MemCrc32(&Room.Footprint, sizeof(Room.Footprint), Hash);
				Hash = FCrc::MemCrc32(&Room.RotationQuarterTurns, sizeof(Room.RotationQuarterTurns), Hash);
				Hash = FCrc::MemCrc32(&Room.PhaseIndex, sizeof(Room.PhaseIndex), Hash);
			}
			for (const FRandomRoomPlannedConnection& Connection : Plan.Connections)
			{
				Hash = FCrc::MemCrc32(&Connection.FirstRoom.Value, sizeof(Connection.FirstRoom.Value), Hash);
				Hash = AddNameToHash(Hash, Connection.FirstConnector);
				Hash = FCrc::MemCrc32(&Connection.SecondRoom.Value, sizeof(Connection.SecondRoom.Value), Hash);
				Hash = AddNameToHash(Hash, Connection.SecondConnector);
			}
			return Hash;
		}
	}

	FRandomRoomLayoutPlan FRandomRoomLayoutPlanner::Generate(const FRandomRoomGenerationRequest& Request)
	{
		FRandomRoomLayoutPlan Result;
		Result.Seed = Request.Seed;
		if (!ValidateRequest(Request, Result.FailureReason))
		{
			return Result;
		}

		const TArray<FRandomRoomPhaseDefinition> SortedPhases = GetSortedPhases(Request.Phases);
		const TArray<int32> PhaseSlots = BuildPhaseSlots(Request);
		for (int32 AttemptIndex = 0; AttemptIndex < Request.MaxGenerationAttempts; ++AttemptIndex)
		{
			FRandomRoomLayoutPlan CandidatePlan;
			CandidatePlan.Seed = Request.Seed;
			CandidatePlan.AttemptIndex = AttemptIndex;
			FRandomStream RandomStream(static_cast<int32>(static_cast<uint32>(Request.Seed) + static_cast<uint32>(AttemptIndex) * 0x9E3779B9u));

			FRandomRoomPlannedRoom StartRoom;
			StartRoom.Handle.Value = 0;
			StartRoom.TemplateId = Request.StartTemplate->TemplateId;
			StartRoom.RoomType = ChooseStartRoomType(*Request.StartTemplate);
			StartRoom.Footprint = Request.StartTemplate->Footprint;
			StartRoom.PhaseIndex = PhaseSlots[0];
			for (const FRandomRoomConnectorDefinition& Connector : Request.StartTemplate->Connectors)
			{
				StartRoom.Connectors.Add(MakeRotatedConnector(Connector, StartRoom.Footprint, 0));
			}
			CandidatePlan.Rooms.Add(StartRoom);

			TSet<FIntPoint> OccupiedCells;
			AddFootprintToOccupiedCells(StartRoom, OccupiedCells);
			TArray<FFrontierConnector> Frontier;
			for (int32 ConnectorIndex = 0; ConnectorIndex < StartRoom.Connectors.Num(); ++ConnectorIndex)
			{
				Frontier.Add({ 0, ConnectorIndex });
			}

			bool bAttemptSucceeded = true;
			while (CandidatePlan.Rooms.Num() < PhaseSlots.Num())
			{
				const int32 NewRoomIndex = CandidatePlan.Rooms.Num();
				const int32 NewRoomPhase = PhaseSlots[NewRoomIndex];
				const FRandomRoomPhaseDefinition* PhaseDefinition = FindPhaseByIndex(SortedPhases, NewRoomPhase);
				FRandomRoomPhaseDefinition DefaultPhase;
				const FRandomRoomPhaseDefinition& Phase = PhaseDefinition ? *PhaseDefinition : DefaultPhase;
				TArray<FPlacementCandidate> PlacementCandidates;

				for (int32 FrontierIndex = 0; FrontierIndex < Frontier.Num(); ++FrontierIndex)
				{
					const FFrontierConnector& FrontierEntry = Frontier[FrontierIndex];
					const FRandomRoomPlannedRoom& ExistingRoom = CandidatePlan.Rooms[FrontierEntry.RoomIndex];
					const FRandomRoomPlannedConnector& ExistingConnector = ExistingRoom.Connectors[FrontierEntry.ConnectorIndex];
					const FIntPoint AdjacentCell = ExistingRoom.Origin + ExistingConnector.Cell + DirectionToCellOffset(ExistingConnector.Direction);

					for (const FRandomRoomTemplateDefinition* Template : Request.Templates)
					{
						if (!Template || Template == Request.StartTemplate || Template->GenerationWeight <= 0.0f)
						{
							continue;
						}
						TArray<FGameplayTag> TemplateRoomTypes;
						Template->AllowedRoomTypes.GetGameplayTagArray(TemplateRoomTypes);
						if (TemplateRoomTypes.IsEmpty())
						{
							// Untagged templates are valid for projects that do not use Gameplay Tags.
							TemplateRoomTypes.Add(FGameplayTag());
						}
						TemplateRoomTypes.Sort([](const FGameplayTag& Left, const FGameplayTag& Right)
						{
							return Left.ToString() < Right.ToString();
						});

						for (const FGameplayTag RoomType : TemplateRoomTypes)
						{
							const float EffectiveWeight = Template->GenerationWeight * GetRoomTypeWeight(Phase, RoomType);
							if (EffectiveWeight <= 0.0f)
							{
								continue;
							}
							for (int32 Rotation = 0; Rotation < 4; ++Rotation)
							{
								const FIntPoint RotatedFootprint = GetRotatedFootprint(Template->Footprint, Rotation);
								for (int32 ConnectorIndex = 0; ConnectorIndex < Template->Connectors.Num(); ++ConnectorIndex)
								{
									const FRandomRoomPlannedConnector RotatedConnector = MakeRotatedConnector(Template->Connectors[ConnectorIndex], Template->Footprint, Rotation);
									if (RotatedConnector.Direction != OppositeDirection(ExistingConnector.Direction)
										|| !AreConnectorTagsCompatible(ExistingConnector.Tags, RotatedConnector.Tags))
									{
										continue;
									}
									const FIntPoint NewOrigin = AdjacentCell - RotatedConnector.Cell;
									if (!DoesFootprintOverlap(NewOrigin, RotatedFootprint, OccupiedCells))
									{
										PlacementCandidates.Add({ FrontierIndex, Template, RoomType, Rotation, ConnectorIndex, NewOrigin, EffectiveWeight });
									}
								}
							}
						}
					}
				}

				PlacementCandidates.Sort([](const FPlacementCandidate& Left, const FPlacementCandidate& Right)
				{
					if (Left.Template->TemplateId != Right.Template->TemplateId)
					{
						return Left.Template->TemplateId.LexicalLess(Right.Template->TemplateId);
					}
					if (Left.RoomType != Right.RoomType)
					{
						return Left.RoomType.ToString() < Right.RoomType.ToString();
					}
					if (Left.RotationQuarterTurns != Right.RotationQuarterTurns)
					{
						return Left.RotationQuarterTurns < Right.RotationQuarterTurns;
					}
					if (Left.FrontierIndex != Right.FrontierIndex)
					{
						return Left.FrontierIndex < Right.FrontierIndex;
					}
					return Left.ConnectorIndex < Right.ConnectorIndex;
				});

				float TotalWeight = 0.0f;
				for (const FPlacementCandidate& PlacementCandidate : PlacementCandidates)
				{
					TotalWeight += PlacementCandidate.Weight;
				}
				if (PlacementCandidates.IsEmpty() || TotalWeight <= 0.0f)
				{
					bAttemptSucceeded = false;
					CandidatePlan.FailureReason = TEXT("No compatible non-overlapping room placement remained.");
					break;
				}

				const float Selection = RandomStream.FRandRange(0.0f, TotalWeight);
				float RunningWeight = 0.0f;
				int32 SelectedIndex = PlacementCandidates.Num() - 1;
				for (int32 CandidateIndex = 0; CandidateIndex < PlacementCandidates.Num(); ++CandidateIndex)
				{
					RunningWeight += PlacementCandidates[CandidateIndex].Weight;
					if (Selection <= RunningWeight)
					{
						SelectedIndex = CandidateIndex;
						break;
					}
				}
				const FPlacementCandidate& Selected = PlacementCandidates[SelectedIndex];
				const FFrontierConnector UsedFrontier = Frontier[Selected.FrontierIndex];
				const FRandomRoomPlannedRoom& ExistingRoom = CandidatePlan.Rooms[UsedFrontier.RoomIndex];

				FRandomRoomPlannedRoom NewRoom;
				NewRoom.Handle.Value = NewRoomIndex;
				NewRoom.TemplateId = Selected.Template->TemplateId;
				NewRoom.RoomType = Selected.RoomType;
				NewRoom.Origin = Selected.Origin;
				NewRoom.Footprint = GetRotatedFootprint(Selected.Template->Footprint, Selected.RotationQuarterTurns);
				NewRoom.RotationQuarterTurns = Selected.RotationQuarterTurns;
				NewRoom.PhaseIndex = NewRoomPhase;
				for (const FRandomRoomConnectorDefinition& Connector : Selected.Template->Connectors)
				{
					NewRoom.Connectors.Add(MakeRotatedConnector(Connector, Selected.Template->Footprint, Selected.RotationQuarterTurns));
				}

				FRandomRoomPlannedConnection& Connection = CandidatePlan.Connections.AddDefaulted_GetRef();
				Connection.FirstRoom = ExistingRoom.Handle;
				Connection.FirstConnector = ExistingRoom.Connectors[UsedFrontier.ConnectorIndex].ConnectorId;
				Connection.SecondRoom = NewRoom.Handle;
				Connection.SecondConnector = NewRoom.Connectors[Selected.ConnectorIndex].ConnectorId;

				Frontier.RemoveAtSwap(Selected.FrontierIndex, EAllowShrinking::No);
				CandidatePlan.Rooms.Add(NewRoom);
				AddFootprintToOccupiedCells(NewRoom, OccupiedCells);
				for (int32 ConnectorIndex = 0; ConnectorIndex < NewRoom.Connectors.Num(); ++ConnectorIndex)
				{
					if (ConnectorIndex != Selected.ConnectorIndex)
					{
						Frontier.Add({ NewRoomIndex, ConnectorIndex });
					}
				}
			}

			if (bAttemptSucceeded)
			{
				CandidatePlan.bSucceeded = true;
				CandidatePlan.StableHash = CalculateStableHash(CandidatePlan);
				FString VerificationFailure;
				if (VerifyPlan(CandidatePlan, VerificationFailure))
				{
					return CandidatePlan;
				}
				CandidatePlan.bSucceeded = false;
				CandidatePlan.FailureReason = VerificationFailure;
			}
			Result.FailureReason = CandidatePlan.FailureReason;
		}

		if (Result.FailureReason.IsEmpty())
		{
			Result.FailureReason = TEXT("Layout generation exhausted all attempts.");
		}
		return Result;
	}

	bool FRandomRoomLayoutPlanner::VerifyPlan(const FRandomRoomLayoutPlan& Plan, FString& OutFailureReason)
	{
		OutFailureReason.Reset();
		if (!Plan.bSucceeded || Plan.Rooms.IsEmpty())
		{
			OutFailureReason = TEXT("Plan was not successfully generated.");
			return false;
		}
		if (Plan.Connections.Num() != Plan.Rooms.Num() - 1)
		{
			OutFailureReason = TEXT("Plan is not a connected placement tree.");
			return false;
		}

		TSet<FIntPoint> OccupiedCells;
		TMap<int32, int32> RoomIndexByHandle;
		for (int32 RoomIndex = 0; RoomIndex < Plan.Rooms.Num(); ++RoomIndex)
		{
			const FRandomRoomPlannedRoom& Room = Plan.Rooms[RoomIndex];
			if (!Room.Handle.IsValid() || RoomIndexByHandle.Contains(Room.Handle.Value))
			{
				OutFailureReason = TEXT("Plan contains invalid or duplicate room handles.");
				return false;
			}
			RoomIndexByHandle.Add(Room.Handle.Value, RoomIndex);
			for (int32 X = 0; X < Room.Footprint.X; ++X)
			{
				for (int32 Y = 0; Y < Room.Footprint.Y; ++Y)
				{
					const FIntPoint OccupiedCell = Room.Origin + FIntPoint(X, Y);
					if (OccupiedCells.Contains(OccupiedCell))
					{
						OutFailureReason = TEXT("Plan contains overlapping room footprints.");
						return false;
					}
					OccupiedCells.Add(OccupiedCell);
				}
			}
		}

		TMap<int32, TArray<int32>> Adjacency;
		for (const FRandomRoomPlannedConnection& Connection : Plan.Connections)
		{
			const int32* FirstRoomIndex = RoomIndexByHandle.Find(Connection.FirstRoom.Value);
			const int32* SecondRoomIndex = RoomIndexByHandle.Find(Connection.SecondRoom.Value);
			if (!FirstRoomIndex || !SecondRoomIndex)
			{
				OutFailureReason = TEXT("Connection references a missing room.");
				return false;
			}
			const FRandomRoomPlannedRoom& FirstRoom = Plan.Rooms[*FirstRoomIndex];
			const FRandomRoomPlannedRoom& SecondRoom = Plan.Rooms[*SecondRoomIndex];
			const FRandomRoomPlannedConnector* FirstConnector = FirstRoom.Connectors.FindByPredicate([&Connection](const FRandomRoomPlannedConnector& Connector)
			{
				return Connector.ConnectorId == Connection.FirstConnector;
			});
			const FRandomRoomPlannedConnector* SecondConnector = SecondRoom.Connectors.FindByPredicate([&Connection](const FRandomRoomPlannedConnector& Connector)
			{
				return Connector.ConnectorId == Connection.SecondConnector;
			});
			if (!FirstConnector || !SecondConnector
				|| FirstConnector->Direction != OppositeDirection(SecondConnector->Direction)
				|| !AreConnectorTagsCompatible(FirstConnector->Tags, SecondConnector->Tags)
				|| FirstRoom.Origin + FirstConnector->Cell + DirectionToCellOffset(FirstConnector->Direction) != SecondRoom.Origin + SecondConnector->Cell)
			{
				OutFailureReason = TEXT("Plan contains an invalid connector connection.");
				return false;
			}
			Adjacency.FindOrAdd(*FirstRoomIndex).Add(*SecondRoomIndex);
			Adjacency.FindOrAdd(*SecondRoomIndex).Add(*FirstRoomIndex);
		}

		TSet<int32> VisitedRooms;
		TArray<int32> PendingRooms = { 0 };
		while (!PendingRooms.IsEmpty())
		{
			const int32 RoomIndex = PendingRooms.Pop(EAllowShrinking::No);
			if (VisitedRooms.Contains(RoomIndex))
			{
				continue;
			}
			VisitedRooms.Add(RoomIndex);
			for (const int32 Neighbor : Adjacency.FindOrAdd(RoomIndex))
			{
				PendingRooms.Add(Neighbor);
			}
		}
		if (VisitedRooms.Num() != Plan.Rooms.Num())
		{
			OutFailureReason = TEXT("Plan contains disconnected rooms.");
			return false;
		}
		return true;
	}

	bool FRandomRoomLayoutPlanner::ValidateRequest(const FRandomRoomGenerationRequest& Request, FString& OutFailureReason)
	{
		OutFailureReason.Reset();
		if (Request.MaxRoomCount <= 0 || Request.MaxGenerationAttempts <= 0 || !Request.StartTemplate)
		{
			OutFailureReason = TEXT("Request requires a start template and positive room/attempt counts.");
			return false;
		}
		TSet<FName> TemplateIds;
		bool bFoundStartTemplate = false;
		for (const FRandomRoomTemplateDefinition* Template : Request.Templates)
		{
			if (!Template)
			{
				OutFailureReason = TEXT("Request contains a null room template.");
				return false;
			}
			if (!ValidateTemplate(*Template, OutFailureReason))
			{
				return false;
			}
			if (TemplateIds.Contains(Template->TemplateId))
			{
				OutFailureReason = TEXT("Room template identifiers must be unique.");
				return false;
			}
			TemplateIds.Add(Template->TemplateId);
			bFoundStartTemplate |= Template == Request.StartTemplate;
		}
		if (!bFoundStartTemplate)
		{
			OutFailureReason = TEXT("Start template must be included in the template collection.");
			return false;
		}
		return true;
	}

	bool FRandomRoomLayoutPlanner::ValidateTemplate(const FRandomRoomTemplateDefinition& Template, FString& OutFailureReason)
	{
		if (Template.TemplateId.IsNone() || Template.Footprint.X <= 0 || Template.Footprint.Y <= 0 || Template.Connectors.IsEmpty())
		{
			OutFailureReason = TEXT("Every room template needs an id, positive footprint, and at least one connector.");
			return false;
		}
		TSet<FName> ConnectorIds;
		for (const FRandomRoomConnectorDefinition& Connector : Template.Connectors)
		{
			if (Connector.ConnectorId.IsNone() || ConnectorIds.Contains(Connector.ConnectorId)
				|| !IsCellInsideFootprint(Connector.Cell, Template.Footprint)
				|| !IsConnectorOnMatchingBoundary(Connector, Template.Footprint))
			{
				OutFailureReason = FString::Printf(TEXT("Template '%s' contains an invalid connector."), *Template.TemplateId.ToString());
				return false;
			}
			ConnectorIds.Add(Connector.ConnectorId);
		}
		return true;
	}
}
