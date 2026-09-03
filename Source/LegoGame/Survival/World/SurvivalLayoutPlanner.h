#pragma once

#include "CoreMinimal.h"
#include "LegoGame/Survival/Contracts/SurvivalDataAssets.h"

namespace LG::Survival::World
{
	struct FPlannedConnector
	{
		FName ConnectorId = NAME_None;
		FIntPoint Cell = FIntPoint::ZeroValue;
		ERoomConnectorDirection Direction = ERoomConnectorDirection::North;
		FGameplayTagContainer Tags;
	};

	struct FPlannedRoom
	{
		FRoomHandle Handle;
		FName TemplateId = NAME_None;
		FGameplayTag RoomType;
		FIntPoint Origin = FIntPoint::ZeroValue;
		FIntPoint Footprint = FIntPoint(1, 1);
		int32 RotationQuarterTurns = 0;
		int32 PhaseIndex = 0;
		TArray<FPlannedConnector> Connectors;
	};

	struct FPlannedConnection
	{
		FRoomHandle FirstRoom;
		FName FirstConnector = NAME_None;
		FRoomHandle SecondRoom;
		FName SecondConnector = NAME_None;
	};

	struct FLayoutPlan
	{
		int32 Seed = 0;
		int32 AttemptIndex = INDEX_NONE;
		uint32 StableHash = 0;
		bool bSucceeded = false;
		FString FailureReason;
		TArray<FPlannedRoom> Rooms;
		TArray<FPlannedConnection> Connections;
		FRoomHandle PoliceTeamStartRoom;
		FRoomHandle BanditTeamStartRoom;
		int32 RequestedTeamStartGraphDistance = 0;
		int32 AppliedTeamStartGraphDistance = 0;
		bool bUsedFarthestTeamStartFallback = false;
	};

	struct FLayoutRequest
	{
		int32 Seed = 0;
		int32 MaxRoomCount = 0;
		int32 MaxGenerationAttempts = 1;
		int32 MinTeamStartGraphDistance = 0;
		const URoomTemplateData* StartTemplate = nullptr;
		TArray<const URoomTemplateData*> Templates;
		TArray<FSurvivalPhaseDefinition> Phases;
	};

	/**
	 * Server-side deterministic grid planner. Its output contains no UObject references,
	 * so the same inputs always yield the same layout hash on every machine.
	 */
	class FSurvivalLayoutPlanner final
	{
	public:
		static FLayoutPlan Generate(const FLayoutRequest& Request);
		static bool VerifyPlan(const FLayoutPlan& Plan, FString& OutFailureReason);
	};
}
