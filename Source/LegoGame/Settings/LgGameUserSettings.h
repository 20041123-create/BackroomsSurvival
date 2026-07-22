// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "LgGameUserSettings.generated.h"

/**
 * 
 */

UCLASS(BlueprintType)//可以被蓝图看见
class LEGOGAME_API ULgGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()
public:
	ULgGameUserSettings();
	
	float GetMouseSpeedRate() const;
	
	
protected:
	
	
	float MouseMinSpeed;
	float MouseMaxSpeed;
	
	UPROPERTY(BlueprintReadWrite,Config)//Config 在usersetting中当作配置文件存到磁盘
	float MouseSpeedRate;
};
