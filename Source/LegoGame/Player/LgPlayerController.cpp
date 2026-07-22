// Fill out your copyright notice in the Description page of Project Settings.


#include "LgPlayerController.h"

#include "LegoGame/Manager/LgPlayerCameraManager.h"

ALgPlayerController::ALgPlayerController()
{
	PlayerCameraManagerClass = ALgPlayerCameraManager::StaticClass();
}
