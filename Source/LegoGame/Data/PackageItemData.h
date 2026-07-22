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
	int32 Key;
	int32 ID;
};
