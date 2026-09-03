#pragma once

#include "CoreMinimal.h"
#include "LegoGame/Survival/Contracts/SurvivalTypes.h"
#include "UObject/Object.h"
#include "SurvivalMatchIntegrationStub.generated.h"

class USurvivalModeConfig;

DECLARE_DELEGATE_OneParam(FSurvivalStubLayoutReady, bool /* bSucceeded */);

/**
 * TEMP_SURVIVAL_INTEGRATION_STUB
 *
 * Development-only replacement for the World/Core runtime bridges that do not
 * yet exist in Survival Contracts. It never creates rooms, items, or monsters.
 */
UCLASS(Transient)
class LEGOGAME_API USurvivalMatchIntegrationStub : public UObject
{
	GENERATED_BODY()

public:
	void RequestLayout(const USurvivalModeConfig& Config, FSurvivalStubLayoutReady Completion);
	int32 UnlockRoomsTo(int32 CumulativeTarget, const TMap<FGameplayTag, float>& RoomTypeWeights);

	bool GetTeamSpawnTransform(UWorld* World, ETeamType TeamType, FTransform& OutTransform) const;
	bool TrySpawnResource();
	bool TrySpawnEnemy(int32 MaxAliveEnemies);
	void NotifyStubEnemyDefeated();

	int32 GetUnlockedRoomCount() const { return UnlockedRoomCount; }
	int32 GetAliveEnemyCount() const { return AliveEnemyCount; }

private:
	struct FStubRoom
	{
		FRoomHandle Handle;
		FGameplayTag RoomType;
		ESurvivalRoomState State = ESurvivalRoomState::Locked;
	};

	FGameplayTag ChooseRoomType(const TMap<FGameplayTag, float>& RoomTypeWeights);

	TArray<FStubRoom> Rooms;
	FRandomStream RandomStream;
	int32 UnlockedRoomCount = 0;
	int32 AliveEnemyCount = 0;
};
