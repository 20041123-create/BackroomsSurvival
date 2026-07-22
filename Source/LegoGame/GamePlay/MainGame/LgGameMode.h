// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LgGameMode.generated.h"

class ALgPlayerStart;
enum class ETeamType : uint8;
/**
 * 
 */
UCLASS()
class LEGOGAME_API ALgGameMode : public AGameMode
{
	GENERATED_BODY()
	ALgGameMode();
	
protected:	
	// UFUNCTION(Exec)
	// void TestFun(int32 N);
	
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;
	
	void GetTeamSpawnTransform(ETeamType TeamType, FTransform& SpawnTransform);
	
protected:
	
	UPROPERTY()
	TMap<ETeamType, ALgPlayerStart*> PlayerStartMap;
};
