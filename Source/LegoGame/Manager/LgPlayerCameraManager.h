// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "LgPlayerCameraManager.generated.h"

class APlayerCharacter;
/**
 * 
 */
UCLASS()
class LEGOGAME_API ALgPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
	
protected:
	virtual void UpdateCamera(float DeltaTime) override;
	
protected:
	bool LastCrouchState;
	float SpringArmZ;
	UPROPERTY()
	TObjectPtr<APlayerCharacter> PlayerCharacter;
};
