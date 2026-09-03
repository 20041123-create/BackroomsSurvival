#include "SurvivalMatchIntegrationStub.h"

#include "EngineUtils.h"
#include "LegoGame/Scene/LgPlayerStart.h"
#include "LegoGame/Survival/Contracts/SurvivalDataAssets.h"
#include "LegoGame/Survival/Contracts/SurvivalGameplayTags.h"

void USurvivalMatchIntegrationStub::RequestLayout(const USurvivalModeConfig& Config, FSurvivalStubLayoutReady Completion)
{
	Rooms.Reset();
	UnlockedRoomCount = 0;
	AliveEnemyCount = 0;
	RandomStream.Initialize(Config.RandomSeed);

	for (int32 RoomIndex = 0; RoomIndex < Config.MaxRoomCount; ++RoomIndex)
	{
		FStubRoom& Room = Rooms.AddDefaulted_GetRef();
		Room.Handle.Value = RoomIndex;
		Room.RoomType = LG::SurvivalTags::Room_Type_Normal;
		Room.State = ESurvivalRoomState::Locked;
	}

	Completion.ExecuteIfBound(Rooms.Num() >= 2);
}

int32 USurvivalMatchIntegrationStub::UnlockRoomsTo(
	int32 CumulativeTarget,
	const TMap<FGameplayTag, float>& RoomTypeWeights)
{
	const int32 ClampedTarget = FMath::Clamp(CumulativeTarget, 0, Rooms.Num());
	for (int32 RoomIndex = UnlockedRoomCount; RoomIndex < ClampedTarget; ++RoomIndex)
	{
		FStubRoom& Room = Rooms[RoomIndex];
		Room.RoomType = RoomIndex < 2
			? LG::SurvivalTags::Room_Type_Normal
			: ChooseRoomType(RoomTypeWeights);
		Room.State = ESurvivalRoomState::Active;
	}

	UnlockedRoomCount = FMath::Max(UnlockedRoomCount, ClampedTarget);
	return UnlockedRoomCount;
}

bool USurvivalMatchIntegrationStub::GetTeamSpawnTransform(
	UWorld* World,
	ETeamType TeamType,
	FTransform& OutTransform) const
{
	if (!World)
	{
		return false;
	}

	for (TActorIterator<ALgPlayerStart> It(World); It; ++It)
	{
		if (It->GetTeamType() == TeamType)
		{
			OutTransform = It->GetTransform();
			return true;
		}
	}

	return false;
}

bool USurvivalMatchIntegrationStub::TrySpawnResource()
{
	return UnlockedRoomCount > 0;
}

bool USurvivalMatchIntegrationStub::TrySpawnEnemy(int32 MaxAliveEnemies)
{
	if (UnlockedRoomCount == 0 || MaxAliveEnemies <= 0 || AliveEnemyCount >= MaxAliveEnemies)
	{
		return false;
	}

	++AliveEnemyCount;
	return true;
}

void USurvivalMatchIntegrationStub::NotifyStubEnemyDefeated()
{
	AliveEnemyCount = FMath::Max(0, AliveEnemyCount - 1);
}

FGameplayTag USurvivalMatchIntegrationStub::ChooseRoomType(const TMap<FGameplayTag, float>& RoomTypeWeights)
{
	struct FWeightedRoomType
	{
		FGameplayTag Tag;
		float Weight = 0.0f;
	};

	TArray<FWeightedRoomType> Choices;
	float TotalWeight = 0.0f;
	for (const TPair<FGameplayTag, float>& Pair : RoomTypeWeights)
	{
		if (Pair.Key.IsValid() && Pair.Value > 0.0f)
		{
			FWeightedRoomType& Choice = Choices.AddDefaulted_GetRef();
			Choice.Tag = Pair.Key;
			Choice.Weight = Pair.Value;
			TotalWeight += Pair.Value;
		}
	}

	if (Choices.IsEmpty() || TotalWeight <= 0.0f)
	{
		return LG::SurvivalTags::Room_Type_Normal;
	}

	Choices.Sort([](const FWeightedRoomType& Left, const FWeightedRoomType& Right)
	{
		return Left.Tag.ToString() < Right.Tag.ToString();
	});

	const float Roll = RandomStream.FRandRange(0.0f, TotalWeight);
	float RunningWeight = 0.0f;
	for (const FWeightedRoomType& Choice : Choices)
	{
		RunningWeight += Choice.Weight;
		if (Roll <= RunningWeight)
		{
			return Choice.Tag;
		}
	}

	return Choices.Last().Tag;
}
