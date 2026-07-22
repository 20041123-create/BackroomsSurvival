// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "AccountSaveGame.generated.h"

/**
 * 
 */
UCLASS()
class LEGOGAME_API UAccountSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY()
	FString AccountString;
	UPROPERTY()
	FString PasswordString;
};
