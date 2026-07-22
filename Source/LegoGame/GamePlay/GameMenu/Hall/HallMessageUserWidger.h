// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HallMessageUserWidger.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class LEGOGAME_API UHallMessageUserWidger : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	UFUNCTION(BlueprintCallable)
	void ShowMessage(FText Title, FText Message);
	
protected:
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TitleTextBlock;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> MessageTextBlock;
};
