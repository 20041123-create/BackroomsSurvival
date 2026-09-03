// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LegoGame/Scene/SceneItemActor.h"
#include "UPackageUserWidget.generated.h"

class APlayerCharacter;
class APlayerModelActor;
class UWeaponFrameWidget;
class UVerticalBox;
enum class ESkinType : uint8;
class UListView;
class UPackageListViewWidget;
class UPackageItemWidget;
class UScrollBox;
class ASceneItemActor;
/**
 * 
 */
UCLASS()
class LEGOGAME_API UUPackageUserWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	virtual void RemoveFromParent() override;

	
	
protected:
	
	virtual void NativeConstruct() override;
	
	void OnAddNearItemActor(ASceneItemActor* SceneItemActor);
	void OnRemoveNearItemActor(ASceneItemActor* SceneItemActor);
	
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	
	void OnPutOnSkin(ESkinType SkinType,int32 ID);
	void OnTakeOffSkin(ESkinType SkinType,int32 ID);
	
	void OnEquipWeapon(int32 ID);
	void OnUnEquipWeapon(int32 ID);
	void OnSurvivalInventoryChanged(const TArray<FItemStack>& Items);
	void OnSurvivalInventorySelectionChanged(int32 SelectedSlotId);
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UScrollBox> NearBox;
	UPROPERTY()
	TSubclassOf<UPackageItemWidget> PackageItemWidgetClass;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UPackageListViewWidget> PackageListViewWidget;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> SkinVerticalBox;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWeaponFrameWidget> WeaponFrame;

	UPROPERTY()
	TObjectPtr<APlayerModelActor>  PlayerModelActor;
	
};
