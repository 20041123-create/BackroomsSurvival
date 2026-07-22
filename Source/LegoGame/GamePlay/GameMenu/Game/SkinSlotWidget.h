// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkinSlotWidget.generated.h"

class UImage;
class UTextBlock;
enum class ESkinType : uint8;
/**
 * 
 */
UCLASS()
class LEGOGAME_API USkinSlotWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	
	ESkinType GetSkinType() const {return SkinType;}
	
	void InitPanel(int32 ID);
	
protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	
	virtual void NativePreConstruct() override;
	
protected:
	
	UPROPERTY(EditAnywhere)
	ESkinType SkinType;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> SkinTextBlock;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> IconImage; 
};
