#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RandomRoomGenerationTypes.generated.h"

UENUM(BlueprintType)
enum class ERandomRoomConnectorDirection : uint8 { North, East, South, West };

UENUM(BlueprintType)
enum class ERandomRoomState : uint8 { Planned, Locked, Active, Cleared };

USTRUCT(BlueprintType)
struct RANDOMROOMGENERATION_API FRandomRoomHandle
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Value = INDEX_NONE;
	bool IsValid() const { return Value != INDEX_NONE; }
	bool operator==(const FRandomRoomHandle& Other) const { return Value == Other.Value; }
	friend uint32 GetTypeHash(const FRandomRoomHandle& Handle) { return GetTypeHash(Handle.Value); }
};

USTRUCT(BlueprintType)
struct RANDOMROOMGENERATION_API FRandomRoomConnectorDefinition
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ConnectorId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FIntPoint Cell = FIntPoint::ZeroValue;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) ERandomRoomConnectorDirection Direction = ERandomRoomConnectorDirection::North;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayTagContainer ConnectorTags;
};

USTRUCT(BlueprintType)
struct RANDOMROOMGENERATION_API FRandomRoomPhaseDefinition
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 PhaseIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 RoomsToUnlock = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FGameplayTag, float> RoomTypeWeights;
};

struct RANDOMROOMGENERATION_API FRandomRoomTemplateDefinition
{
	FName TemplateId = NAME_None;
	FIntPoint Footprint = FIntPoint(1, 1);
	TArray<FRandomRoomConnectorDefinition> Connectors;
	FGameplayTagContainer AllowedRoomTypes;
	float GenerationWeight = 1.0f;
};

struct RANDOMROOMGENERATION_API FRandomRoomPlannedConnector
{
	FName ConnectorId = NAME_None;
	FIntPoint Cell = FIntPoint::ZeroValue;
	ERandomRoomConnectorDirection Direction = ERandomRoomConnectorDirection::North;
	FGameplayTagContainer Tags;
};

struct RANDOMROOMGENERATION_API FRandomRoomPlannedRoom
{
	FRandomRoomHandle Handle;
	FName TemplateId = NAME_None;
	FGameplayTag RoomType;
	FIntPoint Origin = FIntPoint::ZeroValue;
	FIntPoint Footprint = FIntPoint(1, 1);
	int32 RotationQuarterTurns = 0;
	int32 PhaseIndex = 0;
	TArray<FRandomRoomPlannedConnector> Connectors;
};

struct RANDOMROOMGENERATION_API FRandomRoomPlannedConnection
{
	FRandomRoomHandle FirstRoom;
	FName FirstConnector = NAME_None;
	FRandomRoomHandle SecondRoom;
	FName SecondConnector = NAME_None;
};

struct RANDOMROOMGENERATION_API FRandomRoomGenerationRequest
{
	int32 Seed = 0;
	int32 MaxRoomCount = 0;
	int32 MaxGenerationAttempts = 1;
	const FRandomRoomTemplateDefinition* StartTemplate = nullptr;
	TArray<const FRandomRoomTemplateDefinition*> Templates;
	TArray<FRandomRoomPhaseDefinition> Phases;
};

struct RANDOMROOMGENERATION_API FRandomRoomLayoutPlan
{
	int32 Seed = 0;
	int32 AttemptIndex = INDEX_NONE;
	uint32 StableHash = 0;
	bool bSucceeded = false;
	FString FailureReason;
	TArray<FRandomRoomPlannedRoom> Rooms;
	TArray<FRandomRoomPlannedConnection> Connections;
};
