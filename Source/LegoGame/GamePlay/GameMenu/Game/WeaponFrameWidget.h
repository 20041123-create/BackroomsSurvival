// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WeaponFrameWidget.generated.h"

class UImage;
/**
 * 
 */
UCLASS()
class LEGOGAME_API UWeaponFrameWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	void InitPanel(int32 ID);
	
protected:
	
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	
protected:
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> IconImage;
	
};
