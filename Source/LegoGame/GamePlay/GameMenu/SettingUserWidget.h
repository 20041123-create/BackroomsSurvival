// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SettingUserWidget.generated.h"

/**
 * 
 */
class UKeySettingUserWidget;
UCLASS()
class LEGOGAME_API USettingUserWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	
	TObjectPtr<UKeySettingUserWidget> GetKeySettingUserWidget() const{return KeySettingUserWidget;}
	
protected:
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UKeySettingUserWidget> KeySettingUserWidget;
	
	
};
