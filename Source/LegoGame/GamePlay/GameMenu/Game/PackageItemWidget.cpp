// Fill out your copyright notice in the Description page of Project Settings.

#include "PackageItemWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "LegoGame/Components/PackageComponent.h"
#include "LegoGame/Data/PackageItemData.h"
#include "LegoGame/GamePlay/MainGame/LgHUD.h"
#include "LegoGame/Player/PlayerCharacter.h"
#include "LegoGame/Scene/SceneItemActor.h"
#include "LegoGame/Subsystem/PropsSubsystem.h"

FReply UPackageItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (SceneItemActor && InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		if (APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwningPlayerPawn()))
		{
			if (UPackageComponent* Package = Player->GetPackageComponent())
			{
				Package->PickItemFromNear(SceneItemActor);
				return FReply::Handled();
			}
		}
	}

	if (!SceneItemActor && bIsSurvivalStack && InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		if (APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwningPlayerPawn()))
		{
			if (UPackageComponent* Package = Player->GetPackageComponent())
			{
				Package->SetSelectedSurvivalSlotId(SurvivalSlotId);
			}
		}
	}

	return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
}

void UPackageItemWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);
	OutOperation = UWidgetBlueprintLibrary::CreateDragDropOperation(UDragDropOperation::StaticClass());
	OutOperation->Payload = this;
	OutOperation->DefaultDragVisual = CopySelf();
	OutOperation->Pivot = EDragPivot::MouseDown;
}

void UPackageItemWidget::InitPanel(TObjectPtr<ASceneItemActor> InSceneItemActor)
{
	bIsSurvivalStack = false;
	SurvivalSlotId = INDEX_NONE;
	UpdateSelectionHighlight(INDEX_NONE);
	if (IsValid(InSceneItemActor))
	{
		InitPanel(InSceneItemActor->GetID());
		SceneItemActor = InSceneItemActor;
	}
}

void UPackageItemWidget::InitPanel(int32 ID, int32 Quantity)
{
	if (const FPropsBase* PropsBase = GetWorld()->GetGameInstance()->GetSubsystem<UPropsSubsystem>()->GetPropsById(ID))
	{
		const FText DisplayName = Quantity > 1
			? FText::Format(FText::FromString(TEXT("{0} x{1}")), PropsBase->Name, FText::AsNumber(Quantity))
			: PropsBase->Name;
		NameTextBlock->SetText(DisplayName);
		IconImage->SetBrushFromTexture(PropsBase->Icon);
	}
}

UPackageItemWidget* UPackageItemWidget::CopySelf()
{
	if (ALgHUD* HUD = Cast<ALgHUD>(GetOwningPlayer()->GetHUD()))
	{
		UPackageItemWidget* CopyObject = HUD->GetSingleWidgetObject<UPackageItemWidget>(this);
		if (CopyObject)
		{
			CopyObject->NameTextBlock->SetText(NameTextBlock->GetText());
			CopyObject->IconImage->SetBrush(IconImage->GetBrush());
			return CopyObject;
		}
	}
	return nullptr;
}

void UPackageItemWidget::UpdateSelectionHighlight(int32 SelectedSlotId)
{
	const bool bIsSelected = bIsSurvivalStack && SurvivalSlotId != INDEX_NONE && SurvivalSlotId == SelectedSlotId;
	SetColorAndOpacity(bIsSelected
		? FLinearColor(0.45f, 0.80f, 1.0f, 1.0f)
		: FLinearColor::White);
}

void UPackageItemWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	if (UPackageItemData* Data = Cast<UPackageItemData>(ListItemObject))
	{
		SceneItemActor = nullptr;
		InitPanel(Data->ID, Data->Quantity);
		PackageKey = Data->Key;
		SurvivalSlotId = Data->SurvivalSlotId;
		bIsSurvivalStack = Data->bIsSurvivalStack;
		if (APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwningPlayerPawn()))
		{
			if (UPackageComponent* Package = Player->GetPackageComponent())
			{
				UpdateSelectionHighlight(Package->GetSelectedSurvivalSlotId());
				return;
			}
		}
		UpdateSelectionHighlight(INDEX_NONE);
	}
}
