// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KeySettingUserWidget.generated.h"

class UInputAction;
class UScrollBox;
class UCustomKeySaveGame;
//创建表格表头
USTRUCT()
struct FKsyInfoHeader : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere)
	FText KeyDescription;
	UPROPERTY(EditAnywhere)
	FKey Key;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UInputAction> InputAction; 
};

/**
 * 
 */
UCLASS()
class LEGOGAME_API UKeySettingUserWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SaveCustomKey(FName KeyEventName,FKey NewKey);
	
protected:
	
	virtual void NativeOnInitialized() override;
	
	FKey GetCustomKey(FName KeyEventName);
	UFUNCTION(BlueprintCallable)
	void ResetAllKeys();
	
	
	UPROPERTY()
	TObjectPtr<UDataTable> DT_KeyMapping;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UScrollBox> MyScrollBox;
	UPROPERTY()
	TObjectPtr<UCustomKeySaveGame> CustomKeySaveGame;
};
