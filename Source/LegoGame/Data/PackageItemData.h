// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PackageItemData.generated.h"

/**
 * 
 */
UCLASS()
class LEGOGAME_API UPackageItemData : public UObject
{
	GENERATED_BODY()
public:
	int32 Key = INDEX_NONE;
	int32 ID = INDEX_NONE;
	int32 SurvivalSlotId = INDEX_NONE;
	int32 Quantity = 1;
	bool bIsSurvivalStack = false;
};
