// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponFrameWidget.h"

#include "PackageItemWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "LegoGame/Components/PackageComponent.h"
#include "LegoGame/Player/PlayerCharacter.h"
#include "LegoGame/Subsystem/PropsSubsystem.h"

void UWeaponFrameWidget::InitPanel(int32 ID)
{
	if (ID == -1)
	{
		IconImage->SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		IconImage->SetVisibility(ESlateVisibility::Visible);
		const FPropsBase* PropsBase = GetWorld()->GetGameInstance()->GetSubsystem<UPropsSubsystem>()->GetPropsById(ID);
		if (PropsBase)
		{
			IconImage->SetBrushFromTexture(PropsBase->Icon);
		}
	}
}

bool UWeaponFrameWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
                                      UDragDropOperation* InOperation)
{
	if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn()))
	{
		if (PlayerCharacter->GetPackageComponent())
		{
			if (UPackageItemWidget* ItemWidget = Cast<UPackageItemWidget>(InOperation->Payload))
			{
				if (ItemWidget->GetSceneItemActor())
				{
					PlayerCharacter->GetPackageComponent()->EquipWeaponFromNear(ItemWidget->GetSceneItemActor());
				}
				else
				{
					PlayerCharacter->GetPackageComponent()->EquipWeaponFromPackage(ItemWidget->GetPackageKey());
				}
			}
		}
	}
	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}



FReply UWeaponFrameWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!IconImage->IsVisible())
	{
		return FReply::Unhandled();
	}
	
	return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent,this,EKeys::LeftMouseButton).NativeReply;
}



void UWeaponFrameWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	OutOperation = UWidgetBlueprintLibrary::CreateDragDropOperation(UDragDropOperation::StaticClass());
	OutOperation->Payload = this;
	OutOperation->DefaultDragVisual = this;
	OutOperation->Pivot = EDragPivot::MouseDown;
}
