// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "PackageItemWidget.generated.h"

class UImage;
class UTextBlock;
class ASceneItemActor;
/**
 * 
 */
UCLASS()
class LEGOGAME_API UPackageItemWidget : public UUserWidget,public IUserObjectListEntry
{
	GENERATED_BODY()
	
protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	//当开启拖拽检测后，鼠标没有按钮松开，而是进行拖动，则此函数调用
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;//当元素对象被设置的时候回调
	
	void InitPanel(int32 ID);
	
	UPackageItemWidget* CopySelf();
	
public:
	
	void InitPanel(TObjectPtr<ASceneItemActor> InSceneItemActor);
	
	TObjectPtr<ASceneItemActor> GetSceneItemActor() const {return SceneItemActor;}
	
	int32 GetPackageKey() const {return PackageKey;}
protected:
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> NameTextBlock;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> IconImage;
	UPROPERTY()
	TObjectPtr<ASceneItemActor> SceneItemActor;
	
	int32 PackageKey;
};
