// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PackageListViewWidget.generated.h"

class UListView;
/**
 * 
 */
UCLASS()
class LEGOGAME_API UPackageListViewWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void OnAddItemActorToPackage(int32 Key,int32 ID);
	void OnRemoveItemActorFromPackage(int32 Key,int32 ID);
	void RefreshItems(const TMap<int32,int32>& Items);
	
protected:
	//响应拖拽抬起
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UListView> MyListView;
};
