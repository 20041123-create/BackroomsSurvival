// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFetureUserWidget.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class LEGOGAME_API UGameFetureUserWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	
	virtual void NativeOnInitialized() override;
	
	void OnEquipWeapon(int32 ID);
	void OnUnEquipWeapon(int32 ID);
	
	void OnWeaponClipChanged(int32 CurrClipVolume,int32 MaxClipVolume);
	
protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> WeaponClipTextBlock; 
	
};
