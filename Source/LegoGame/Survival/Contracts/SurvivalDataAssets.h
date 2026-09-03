#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SurvivalTypes.h"
#include "SurvivalDataAssets.generated.h"

UCLASS(BlueprintType)
class LEGOGAME_API URoomTemplateData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Room")
	FName TemplateId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Room")
	TSoftClassPtr<AActor> RoomActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Room")
	FIntPoint Footprint = FIntPoint(1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Room")
	TArray<FRoomConnectorDefinition> Connectors;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Room")
	FGameplayTagContainer AllowedRoomTypes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Room", meta=(ClampMin="0.0"))
	float GenerationWeight = 1.0f;
};

UCLASS(BlueprintType)
class LEGOGAME_API USurvivalModeConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Generation")
	int32 RandomSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Generation", meta=(ClampMin="2"))
	int32 MaxRoomCount = 32;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Generation", meta=(ClampMin="0"))
	int32 MinTeamStartGraphDistance = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Generation", meta=(ClampMin="1"))
	int32 MaxGenerationAttempts = 32;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Difficulty")
	TArray<FSurvivalPhaseDefinition> Phases;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat", meta=(ClampMin="0.0", ClampMax="1.0"))
	float FriendlyFireDamageScale = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Respawn", meta=(ClampMin="1"))
	int32 RespawnEnergyCost = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Respawn", meta=(ClampMin="0.0"))
	float RespawnDelaySeconds = 15.0f;
};
