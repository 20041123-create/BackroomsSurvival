// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "HallGameMode.generated.h"

/**
 * 
 */
UCLASS()
class LEGOGAME_API AHallGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:	
	
	AHallGameMode();
	
	void KickPlayer(APlayerController* PlayerController);
	
	void EndLgGame();
	
	void StartLgGame();

};
