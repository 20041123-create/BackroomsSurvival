#include "SurvivalLayoutPlanner.h"

#include "Misc/Crc.h"
#include "RandomRoomLayoutPlanner.h"

namespace LG::Survival::World
{
	namespace
	{
		struct FTeamStartSelection
		{
			FRoomHandle PoliceRoom;
			FRoomHandle BanditRoom;
			int32 AppliedDistance = 0;
			bool bUsedFarthestFallback = false;
		};

		struct FRoomPairDistance
		{
			int32 FirstRoom = INDEX_NONE;
			int32 SecondRoom = INDEX_NONE;
			int32 Distance = 0;
		};

		bool ResolveTeamStartSelection(
			const FLayoutPlan& Plan,
			const int32 RequestedDistance,
			FTeamStartSelection& OutSelection,
			FString& OutFailureReason)
		{
			OutSelection = FTeamStartSelection();
			OutFailureReason.Reset();
			if (RequestedDistance < 0)
			{
				OutFailureReason = TEXT("MinTeamStartGraphDistance cannot be negative.");
				return false;
			}
			if (Plan.Rooms.Num() < 2)
			{
				OutFailureReason = TEXT("Team start selection requires at least two planned rooms.");
				return false;
			}

			int32 InitialPhaseIndex = Plan.Rooms[0].PhaseIndex;
			for (const FPlannedRoom& Room : Plan.Rooms)
			{
				InitialPhaseIndex = FMath::Min(InitialPhaseIndex, Room.PhaseIndex);
			}

			TArray<int32> EligibleRooms;
			TSet<int32> EligibleRoomSet;
			for (const FPlannedRoom& Room : Plan.Rooms)
			{
				if (Room.PhaseIndex == InitialPhaseIndex && Room.Handle.IsValid())
				{
					EligibleRooms.Add(Room.Handle.Value);
					EligibleRoomSet.Add(Room.Handle.Value);
				}
			}
			EligibleRooms.Sort();
			if (EligibleRooms.Num() < 2)
			{
				OutFailureReason = TEXT("Initial Survival phase must contain at least two rooms for team starts.");
				return false;
			}

			TMap<int32, TArray<int32>> Adjacency;
			for (const int32 RoomHandle : EligibleRooms)
			{
				Adjacency.Add(RoomHandle);
			}
			for (const FPlannedConnection& Connection : Plan.Connections)
			{
				const int32 FirstRoom = Connection.FirstRoom.Value;
				const int32 SecondRoom = Connection.SecondRoom.Value;
				if (EligibleRoomSet.Contains(FirstRoom) && EligibleRoomSet.Contains(SecondRoom))
				{
					Adjacency.FindChecked(FirstRoom).Add(SecondRoom);
					Adjacency.FindChecked(SecondRoom).Add(FirstRoom);
				}
			}
			for (TPair<int32, TArray<int32>>& Entry : Adjacency)
			{
				Entry.Value.Sort();
			}

			TArray<FRoomPairDistance> ExactPairs;
			TArray<FRoomPairDistance> FarthestPairs;
			int32 FarthestDistance = INDEX_NONE;
			for (int32 SourceIndex = 0; SourceIndex < EligibleRooms.Num(); ++SourceIndex)
			{
				const int32 SourceRoom = EligibleRooms[SourceIndex];
				TMap<int32, int32> Distances;
				TArray<int32> PendingRooms = {SourceRoom};
				Distances.Add(SourceRoom, 0);
				for (int32 PendingIndex = 0; PendingIndex < PendingRooms.Num(); ++PendingIndex)
				{
					const int32 CurrentRoom = PendingRooms[PendingIndex];
					const int32 CurrentDistance = Distances.FindChecked(CurrentRoom);
					for (const int32 Neighbor : Adjacency.FindChecked(CurrentRoom))
					{
						if (!Distances.Contains(Neighbor))
						{
							Distances.Add(Neighbor, CurrentDistance + 1);
							PendingRooms.Add(Neighbor);
						}
					}
				}

				if (Distances.Num() != EligibleRooms.Num())
				{
					OutFailureReason = TEXT("Initial Survival phase room graph is disconnected.");
					return false;
				}

				for (int32 TargetIndex = SourceIndex + 1; TargetIndex < EligibleRooms.Num(); ++TargetIndex)
				{
					FRoomPairDistance Pair;
					Pair.FirstRoom = SourceRoom;
					Pair.SecondRoom = EligibleRooms[TargetIndex];
					Pair.Distance = Distances.FindChecked(Pair.SecondRoom);
					if (RequestedDistance > 0 && Pair.Distance == RequestedDistance)
					{
						ExactPairs.Add(Pair);
					}
					if (Pair.Distance > FarthestDistance)
					{
						FarthestDistance = Pair.Distance;
						FarthestPairs.Reset();
						FarthestPairs.Add(Pair);
					}
					else if (Pair.Distance == FarthestDistance)
					{
						FarthestPairs.Add(Pair);
					}
				}
			}

			const bool bUseExactPair = RequestedDistance > 0 && !ExactPairs.IsEmpty();
			const TArray<FRoomPairDistance>& CandidatePairs = bUseExactPair ? ExactPairs : FarthestPairs;
			if (CandidatePairs.IsEmpty())
			{
				OutFailureReason = TEXT("Initial Survival phase has no valid team-start room pair.");
				return false;
			}

			const FRoomPairDistance& SelectedPair = CandidatePairs[0];
			OutSelection.PoliceRoom.Value = SelectedPair.FirstRoom;
			OutSelection.BanditRoom.Value = SelectedPair.SecondRoom;
			OutSelection.AppliedDistance = SelectedPair.Distance;
			OutSelection.bUsedFarthestFallback = RequestedDistance > 0 && !bUseExactPair;
			return true;
		}

		uint32 AddTeamStartSelectionToHash(const uint32 LayoutHash, const FLayoutPlan& Plan)
		{
			uint32 Hash = LayoutHash;
			Hash = FCrc::MemCrc32(&Plan.RequestedTeamStartGraphDistance, sizeof(Plan.RequestedTeamStartGraphDistance), Hash);
			Hash = FCrc::MemCrc32(&Plan.AppliedTeamStartGraphDistance, sizeof(Plan.AppliedTeamStartGraphDistance), Hash);
			Hash = FCrc::MemCrc32(&Plan.PoliceTeamStartRoom.Value, sizeof(Plan.PoliceTeamStartRoom.Value), Hash);
			Hash = FCrc::MemCrc32(&Plan.BanditTeamStartRoom.Value, sizeof(Plan.BanditTeamStartRoom.Value), Hash);
			return Hash;
		}

		FLayoutPlan MakeFailedPlan(const int32 Seed, const FString& FailureReason)
		{
			FLayoutPlan Result;
			Result.Seed = Seed;
			Result.FailureReason = FailureReason;
			return Result;
		}

		ERandomRoomConnectorDirection ToPluginDirection(const ERoomConnectorDirection Direction)
		{
			return static_cast<ERandomRoomConnectorDirection>(Direction);
		}

		ERoomConnectorDirection ToSurvivalDirection(const ERandomRoomConnectorDirection Direction)
		{
			return static_cast<ERoomConnectorDirection>(Direction);
		}

		FRandomRoomTemplateDefinition MakePluginTemplate(const URoomTemplateData& Template)
		{
			FRandomRoomTemplateDefinition Result;
			Result.TemplateId = Template.TemplateId;
			Result.Footprint = Template.Footprint;
			Result.AllowedRoomTypes = Template.AllowedRoomTypes;
			Result.GenerationWeight = Template.GenerationWeight;
			for (const FRoomConnectorDefinition& Connector : Template.Connectors)
			{
				FRandomRoomConnectorDefinition& PluginConnector = Result.Connectors.AddDefaulted_GetRef();
				PluginConnector.ConnectorId = Connector.ConnectorId;
				PluginConnector.Cell = Connector.Cell;
				PluginConnector.Direction = ToPluginDirection(Connector.Direction);
				PluginConnector.ConnectorTags = Connector.ConnectorTags;
			}
			return Result;
		}

		FRandomRoomPhaseDefinition MakePluginPhase(const FSurvivalPhaseDefinition& Phase)
		{
			FRandomRoomPhaseDefinition Result;
			Result.PhaseIndex = Phase.PhaseIndex;
			Result.RoomsToUnlock = Phase.RoomsToUnlock;
			Result.RoomTypeWeights = Phase.RoomTypeWeights;
			return Result;
		}

		FLayoutPlan MakeSurvivalPlan(const FRandomRoomLayoutPlan& PluginPlan)
		{
			FLayoutPlan Result;
			Result.Seed = PluginPlan.Seed;
			Result.AttemptIndex = PluginPlan.AttemptIndex;
			Result.StableHash = PluginPlan.StableHash;
			Result.bSucceeded = PluginPlan.bSucceeded;
			Result.FailureReason = PluginPlan.FailureReason;
			for (const FRandomRoomPlannedRoom& PluginRoom : PluginPlan.Rooms)
			{
				FPlannedRoom& Room = Result.Rooms.AddDefaulted_GetRef();
				Room.Handle.Value = PluginRoom.Handle.Value;
				Room.TemplateId = PluginRoom.TemplateId;
				Room.RoomType = PluginRoom.RoomType;
				Room.Origin = PluginRoom.Origin;
				Room.Footprint = PluginRoom.Footprint;
				Room.RotationQuarterTurns = PluginRoom.RotationQuarterTurns;
				Room.PhaseIndex = PluginRoom.PhaseIndex;
				for (const FRandomRoomPlannedConnector& PluginConnector : PluginRoom.Connectors)
				{
					FPlannedConnector& Connector = Room.Connectors.AddDefaulted_GetRef();
					Connector.ConnectorId = PluginConnector.ConnectorId;
					Connector.Cell = PluginConnector.Cell;
					Connector.Direction = ToSurvivalDirection(PluginConnector.Direction);
					Connector.Tags = PluginConnector.Tags;
				}
			}
			for (const FRandomRoomPlannedConnection& PluginConnection : PluginPlan.Connections)
			{
				FPlannedConnection& Connection = Result.Connections.AddDefaulted_GetRef();
				Connection.FirstRoom.Value = PluginConnection.FirstRoom.Value;
				Connection.FirstConnector = PluginConnection.FirstConnector;
				Connection.SecondRoom.Value = PluginConnection.SecondRoom.Value;
				Connection.SecondConnector = PluginConnection.SecondConnector;
			}
			return Result;
		}

		FRandomRoomLayoutPlan MakePluginPlan(const FLayoutPlan& SurvivalPlan)
		{
			FRandomRoomLayoutPlan Result;
			Result.Seed = SurvivalPlan.Seed;
			Result.AttemptIndex = SurvivalPlan.AttemptIndex;
			Result.StableHash = SurvivalPlan.StableHash;
			Result.bSucceeded = SurvivalPlan.bSucceeded;
			Result.FailureReason = SurvivalPlan.FailureReason;
			for (const FPlannedRoom& SurvivalRoom : SurvivalPlan.Rooms)
			{
				FRandomRoomPlannedRoom& Room = Result.Rooms.AddDefaulted_GetRef();
				Room.Handle.Value = SurvivalRoom.Handle.Value;
				Room.TemplateId = SurvivalRoom.TemplateId;
				Room.RoomType = SurvivalRoom.RoomType;
				Room.Origin = SurvivalRoom.Origin;
				Room.Footprint = SurvivalRoom.Footprint;
				Room.RotationQuarterTurns = SurvivalRoom.RotationQuarterTurns;
				Room.PhaseIndex = SurvivalRoom.PhaseIndex;
				for (const FPlannedConnector& SurvivalConnector : SurvivalRoom.Connectors)
				{
					FRandomRoomPlannedConnector& Connector = Room.Connectors.AddDefaulted_GetRef();
					Connector.ConnectorId = SurvivalConnector.ConnectorId;
					Connector.Cell = SurvivalConnector.Cell;
					Connector.Direction = ToPluginDirection(SurvivalConnector.Direction);
					Connector.Tags = SurvivalConnector.Tags;
				}
			}
			for (const FPlannedConnection& SurvivalConnection : SurvivalPlan.Connections)
			{
				FRandomRoomPlannedConnection& Connection = Result.Connections.AddDefaulted_GetRef();
				Connection.FirstRoom.Value = SurvivalConnection.FirstRoom.Value;
				Connection.FirstConnector = SurvivalConnection.FirstConnector;
				Connection.SecondRoom.Value = SurvivalConnection.SecondRoom.Value;
				Connection.SecondConnector = SurvivalConnection.SecondConnector;
			}
			return Result;
		}
	}

	FLayoutPlan FSurvivalLayoutPlanner::Generate(const FLayoutRequest& Request)
	{
		if (Request.MinTeamStartGraphDistance < 0)
		{
			return MakeFailedPlan(Request.Seed, TEXT("MinTeamStartGraphDistance cannot be negative."));
		}

		TArray<FRandomRoomTemplateDefinition> PluginTemplates;
		PluginTemplates.Reserve(Request.Templates.Num());
		for (const URoomTemplateData* Template : Request.Templates)
		{
			if (Template)
			{
				PluginTemplates.Add(MakePluginTemplate(*Template));
			}
		}

		FRandomRoomGenerationRequest PluginRequest;
		PluginRequest.Seed = Request.Seed;
		PluginRequest.MaxRoomCount = Request.MaxRoomCount;
		PluginRequest.MaxGenerationAttempts = Request.MaxGenerationAttempts;
		for (const FRandomRoomTemplateDefinition& Template : PluginTemplates)
		{
			PluginRequest.Templates.Add(&Template);
			if (Request.StartTemplate && Template.TemplateId == Request.StartTemplate->TemplateId)
			{
				PluginRequest.StartTemplate = &Template;
			}
		}
		for (const FSurvivalPhaseDefinition& Phase : Request.Phases)
		{
			PluginRequest.Phases.Add(MakePluginPhase(Phase));
		}
		FLayoutPlan Result = MakeSurvivalPlan(RandomRoomGeneration::FRandomRoomLayoutPlanner::Generate(PluginRequest));
		if (!Result.bSucceeded)
		{
			return Result;
		}

		FTeamStartSelection Selection;
		FString SelectionFailureReason;
		if (!ResolveTeamStartSelection(Result, Request.MinTeamStartGraphDistance, Selection, SelectionFailureReason))
		{
			return MakeFailedPlan(Request.Seed, SelectionFailureReason);
		}

		Result.PoliceTeamStartRoom = Selection.PoliceRoom;
		Result.BanditTeamStartRoom = Selection.BanditRoom;
		Result.RequestedTeamStartGraphDistance = Request.MinTeamStartGraphDistance;
		Result.AppliedTeamStartGraphDistance = Selection.AppliedDistance;
		Result.bUsedFarthestTeamStartFallback = Selection.bUsedFarthestFallback;
		Result.StableHash = AddTeamStartSelectionToHash(Result.StableHash, Result);
		return Result;
	}

	bool FSurvivalLayoutPlanner::VerifyPlan(const FLayoutPlan& Plan, FString& OutFailureReason)
	{
		if (!RandomRoomGeneration::FRandomRoomLayoutPlanner::VerifyPlan(MakePluginPlan(Plan), OutFailureReason))
		{
			return false;
		}

		FTeamStartSelection ExpectedSelection;
		if (!ResolveTeamStartSelection(
			Plan, Plan.RequestedTeamStartGraphDistance, ExpectedSelection, OutFailureReason))
		{
			return false;
		}
		if (Plan.PoliceTeamStartRoom.Value != ExpectedSelection.PoliceRoom.Value
			|| Plan.BanditTeamStartRoom.Value != ExpectedSelection.BanditRoom.Value
			|| Plan.AppliedTeamStartGraphDistance != ExpectedSelection.AppliedDistance
			|| Plan.bUsedFarthestTeamStartFallback != ExpectedSelection.bUsedFarthestFallback)
		{
			OutFailureReason = TEXT("Plan contains inconsistent team-start room selection metadata.");
			return false;
		}
		return true;
	}
}
