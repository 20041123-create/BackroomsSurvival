// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HallHUD.generated.h"

class UHallUserWidget;
/**
 * 
 */
UCLASS()
class LEGOGAME_API AHallHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	
	UHallUserWidget* GetHallUserWidget() const{return HallUserWidget;}
	
protected:
	
	virtual void BeginPlay() override;
	
protected:
	UPROPERTY()
	TObjectPtr<UHallUserWidget> HallUserWidget; 
};
