// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KeyInfoItem.generated.h"

/**
 * 
 */

struct FKsyInfoHeader; 
class UTextBlock;
class UInputKeySelector;

UCLASS()
class LEGOGAME_API UKeyInfoItem : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	void InitPanel(FName OutKeyEventName,FText KeyDescribe,FKey Key);
	FName GetKeyEventName() const{return KeyEventName;}
	void ResetKey(FKey DefaultKey);
	
protected:
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UInputKeySelector> InputKeySelector;
	
	FName KeyEventName;
	FKey CurrentKey;
protected:
	virtual void NativeOnInitialized() override;
	//绑定动态代理必须加UFunction
	UFUNCTION()
	void OnKeySelected(FInputChord SelectKey);
};
