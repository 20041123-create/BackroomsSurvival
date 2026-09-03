#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "GameplayTagContainer.h"
#include "LegoGame/LegoGame.h"
#include "SurvivalTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class ESurvivalMatchPhase : uint8
{
	WaitingForLayout,
	Countdown,
	InProgress,
	PostMatch
};

/** Authoritative lifecycle state for initial layout generation and phase expansion. */
UENUM(BlueprintType)
enum class ESurvivalWorldLayoutStatus : uint8
{
	NotRequested,
	Generating,
	Succeeded,
	Failed
};

/** Outcome of one server-authoritative request to create a runtime resource or enemy. */
UENUM(BlueprintType)
enum class ESurvivalRuntimeSpawnResultCode : uint8
{
	/** Safe default before a provider has handled a request. */
	Uninitialized,
	Succeeded,
	RejectedNotAuthority,
	InvalidRequest,
	NoMatchingDefinition,
	SpawnBlocked,
	SpawnFailed,
	/** Used by Match when no unique runtime spawn provider can be discovered. */
	ProviderUnavailable
};

UENUM(BlueprintType)
enum class ESurvivalLifeState : uint8
{
	Alive,
	WaitingRespawn,
	Respawning,
	Spectating,
	Eliminated
};

UENUM(BlueprintType)
enum class ESurvivalRoomState : uint8
{
	Planned,
	Locked,
	Active,
	Cleared
};

UENUM(BlueprintType)
enum class ERoomConnectorDirection : uint8
{
	North,
	East,
	South,
	West
};

USTRUCT(BlueprintType)
struct LEGOGAME_API FItemStack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SlotId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ItemId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0"))
	int32 Quantity = 0;

	bool IsValid() const
	{
		return ItemId != INDEX_NONE && Quantity > 0;
	}
};

/**
 * Public, World-implementation-free summary of the generated Survival layout.
 * Match code must use this snapshot instead of reading World-private layout types.
 */
USTRUCT(BlueprintType)
struct LEGOGAME_API FSurvivalWorldRuntimeSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	ESurvivalWorldLayoutStatus LayoutStatus = ESurvivalWorldLayoutStatus::NotRequested;

	UPROPERTY(BlueprintReadOnly)
	bool bSucceeded = false;

	UPROPERTY(BlueprintReadOnly)
	int32 AppliedSeed = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 LayoutHash = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 CurrentUnlockedPhaseIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, meta=(ClampMin="0"))
	int32 MaterializedRoomCount = 0;

	UPROPERTY(BlueprintReadOnly)
	FString FailureReason;
};

USTRUCT(BlueprintType)
struct LEGOGAME_API FSurvivalVitalsSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	float Health = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float MaxHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float Hunger = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float MaxHunger = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float Thirst = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float MaxThirst = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	ESurvivalLifeState LifeState = ESurvivalLifeState::Alive;
};

/**
 * Read-only equipped-weapon ammunition state for Survival presentation code.
 * Match and HUD consumers use this view instead of depending on Core weapon or
 * inventory implementation classes.
 */
USTRUCT(BlueprintType)
struct LEGOGAME_API FSurvivalWeaponAmmoSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bHasEquippedWeapon = false;

	UPROPERTY(BlueprintReadOnly, meta=(ClampMin="0"))
	int32 LoadedAmmo = 0;

	UPROPERTY(BlueprintReadOnly, meta=(ClampMin="0"))
	int32 ClipCapacity = 0;

	UPROPERTY(BlueprintReadOnly, meta=(ClampMin="0"))
	int32 ReserveAmmo = 0;
};

USTRUCT(BlueprintType)
struct LEGOGAME_API FSurvivalItemView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FItemStack Stack;

	UPROPERTY(BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(BlueprintReadOnly)
	FGameplayTagContainer ItemTags;

	UPROPERTY(BlueprintReadOnly, meta=(ClampMin="1"))
	int32 MaxStackSize = 1;
};

USTRUCT(BlueprintType)
struct LEGOGAME_API FSurvivalRecipeDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName RecipeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	// SlotId is ignored for stacks used as recipe ingredients or results.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FItemStack> Ingredients;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FItemStack> Results;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag RequiredStationTag;
};

USTRUCT(BlueprintType)
struct LEGOGAME_API FRoomHandle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Value = INDEX_NONE;

	bool IsValid() const
	{
		return Value != INDEX_NONE;
	}

	bool operator==(const FRoomHandle& Other) const
	{
		return Value == Other.Value;
	}

	friend uint32 GetTypeHash(const FRoomHandle& Handle)
	{
		return GetTypeHash(Handle.Value);
	}
};

/** Match-owned resource spawn intent. Core resolves ItemTag to a concrete item definition. */
USTRUCT(BlueprintType)
struct LEGOGAME_API FSurvivalResourceSpawnRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ItemTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1"))
	int32 Quantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform SpawnTransform = FTransform::Identity;

	/** Public room context only; this never exposes a World-private room object. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRoomHandle RoomHandle;
};

/** Match-owned enemy spawn intent. An invalid EnemyArchetypeTag requests the Core default archetype. */
USTRUCT(BlueprintType)
struct LEGOGAME_API FSurvivalEnemySpawnRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag EnemyArchetypeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform SpawnTransform = FTransform::Identity;

	/** Public room context only; this never exposes a World-private room object. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRoomHandle RoomHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.01"))
	float DifficultyMultiplier = 1.0f;
};

/**
 * Public result from a runtime spawn provider. A successful result requires
 * bSucceeded, the Succeeded code, and a valid SpawnedActor. Failed results
 * always leave SpawnedActor empty.
 */
USTRUCT(BlueprintType)
struct LEGOGAME_API FSurvivalRuntimeSpawnResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bSucceeded = false;

	UPROPERTY(BlueprintReadOnly)
	ESurvivalRuntimeSpawnResultCode ResultCode = ESurvivalRuntimeSpawnResultCode::Uninitialized;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> SpawnedActor = nullptr;

	/** The concrete item or enemy tag selected by Core, when resolution completed. */
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag ResolvedGameplayTag;

	/** Diagnostic only; Match must branch on bSucceeded and ResultCode. */
	UPROPERTY(BlueprintReadOnly)
	FString FailureReason;
};

/** Public anchor data for Match and other consumers that must not know the Anchor Actor class. */
USTRUCT(BlueprintType)
struct LEGOGAME_API FSurvivalAnchorView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag AnchorTag;

	UPROPERTY(BlueprintReadOnly)
	FRoomHandle RoomHandle;

	UPROPERTY(BlueprintReadOnly)
	FTransform Transform = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly)
	bool bEnabled = false;

	// ETT_None indicates that this anchor is not team-specific.
	UPROPERTY(BlueprintReadOnly)
	ETeamType TeamType = ETeamType::ETT_None;
};

USTRUCT(BlueprintType)
struct LEGOGAME_API FRoomConnectorDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ConnectorId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FIntPoint Cell = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ERoomConnectorDirection Direction = ERoomConnectorDirection::North;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTagContainer ConnectorTags;
};

USTRUCT(BlueprintType)
struct LEGOGAME_API FSurvivalPhaseDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 PhaseIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0"))
	float StartTimeSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 RoomsToUnlock = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 ResourceBudget = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 EnemyBudget = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 MaxAliveEnemies = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0"))
	float HungerDrainMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0"))
	float ThirstDrainMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FGameplayTag, float> RoomTypeWeights;
};

USTRUCT(BlueprintType)
struct LEGOGAME_API FTeamSurvivalState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ETeamType TeamType = ETeamType::ETT_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 RespawnEnergy = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 AlivePlayers = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 WaitingPlayers = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bEliminated = false;
};
