// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameMenuGameMode.generated.h"

/**
 * 
 */
UCLASS()
class LEGOGAME_API AGameMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()
	AGameMenuGameMode();
	
	
	protected:
	UFUNCTION(Exec)
	void TestFunc();
};
