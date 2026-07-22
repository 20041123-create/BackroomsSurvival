// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "LgGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class LEGOGAME_API ULgGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	
	void SetErrorMessage(const FText& InErrorMessage);
	
	FString GetMapName() const {return MapName;}
	
protected:
	
	UPROPERTY(BlueprintReadWrite)
	FText ErrorMessage;
	
	UPROPERTY(BlueprintReadWrite)
	FString MapName;
};
