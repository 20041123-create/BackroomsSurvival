#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RandomRoomGenerationTypes.h"
#include "RandomRoomGenerationDataAssets.generated.h"

class AActor;

/** Reusable authoring data for a room template. Projects may attach any Actor subclass as its visual implementation. */
UCLASS(BlueprintType)
class RANDOMROOMGENERATION_API URandomRoomTemplateData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Random Room") FName TemplateId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Random Room") FIntPoint Footprint = FIntPoint(1, 1);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Random Room") TArray<FRandomRoomConnectorDefinition> Connectors;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Random Room") FGameplayTagContainer AllowedRoomTypes;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Random Room", meta=(ClampMin="0.0")) float GenerationWeight = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Random Room") TSoftClassPtr<AActor> RoomActorClass;

	FRandomRoomTemplateDefinition MakeDefinition() const;
};

/** Reusable deterministic generation settings. Runtime callers turn this asset into an FRandomRoomGenerationRequest. */
UCLASS(BlueprintType)
class RANDOMROOMGENERATION_API URandomRoomGenerationConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Random Room") int32 RandomSeed = 1337;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Random Room", meta=(ClampMin="1")) int32 MaxRoomCount = 32;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Random Room", meta=(ClampMin="1")) int32 MaxGenerationAttempts = 32;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Random Room") TArray<FRandomRoomPhaseDefinition> Phases;
};
