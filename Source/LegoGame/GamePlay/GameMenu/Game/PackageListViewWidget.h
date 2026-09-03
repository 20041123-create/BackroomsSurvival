// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PackageListViewWidget.generated.h"

class UListView;
class UPackageComponent;
struct FPackageListViewWidgetTestAccess;
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
	void RefreshItems();
	void UpdateSurvivalSelectionHighlight(int32 SelectedSlotId);
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//响应拖拽抬起
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	void HandleItemDoubleClicked(UObject* ListItem);
	bool RequestUseSurvivalConsumable(UObject* ListItem, UPackageComponent* Package);

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UListView> MyListView;

	friend struct FPackageListViewWidgetTestAccess;
};
