// Fill out your copyright notice in the Description page of Project Settings.


#include "LgGameUserSettings.h"

ULgGameUserSettings::ULgGameUserSettings()
{
	MouseMinSpeed = 10.f;
	MouseMaxSpeed = 60.f;
	MouseSpeedRate = 0.5f;
}

float ULgGameUserSettings::GetMouseSpeedRate() const
{
	return MouseMinSpeed+(MouseMaxSpeed-MouseMinSpeed)*MouseSpeedRate;
}
