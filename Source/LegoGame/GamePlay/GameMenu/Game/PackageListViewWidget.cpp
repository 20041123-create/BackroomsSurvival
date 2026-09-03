// Fill out your copyright notice in the Description page of Project Settings.

#include "PackageListViewWidget.h"

#include "PackageItemWidget.h"
#include "SkinSlotWidget.h"
#include "WeaponFrameWidget.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/ListView.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "LegoGame/Components/PackageComponent.h"
#include "LegoGame/Data/PackageItemData.h"
#include "LegoGame/Player/PlayerCharacter.h"
#include "LegoGame/Subsystem/PropsSubsystem.h"

void UPackageListViewWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (MyListView)
	{
		MyListView->OnItemDoubleClicked().RemoveAll(this);
		MyListView->OnItemDoubleClicked().AddUObject(this, &ThisClass::HandleItemDoubleClicked);
	}
}

void UPackageListViewWidget::NativeDestruct()
{
	if (MyListView)
	{
		MyListView->OnItemDoubleClicked().RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UPackageListViewWidget::HandleItemDoubleClicked(UObject* ListItem)
{
	if (!MyListView || !ListItem)
	{
		return;
	}

	APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwningPlayerPawn());
	UPackageComponent* Package = Player ? Player->GetPackageComponent() : nullptr;
	if (RequestUseSurvivalConsumable(ListItem, Package))
	{
		const UPackageItemData* ItemData = CastChecked<UPackageItemData>(ListItem);
		UE_LOG(LogTemp, Display, TEXT("Survival consumable double-click request sent for ItemId=%d SlotId=%d."),
			ItemData->ID, ItemData->SurvivalSlotId);
	}
}

bool UPackageListViewWidget::RequestUseSurvivalConsumable(UObject* ListItem, UPackageComponent* Package)
{
	const UPackageItemData* ItemData = Cast<UPackageItemData>(ListItem);
	if (!ItemData || !ItemData->bIsSurvivalStack || ItemData->SurvivalSlotId == INDEX_NONE
		|| ItemData->ID == INDEX_NONE || ItemData->Quantity <= 0 || !Package)
	{
		return false;
	}

	FSurvivalItemView CurrentItem;
	if (!Package->GetSurvivalInventoryItem(ItemData->SurvivalSlotId, CurrentItem)
		|| CurrentItem.Stack.ItemId != ItemData->ID || CurrentItem.Stack.Quantity <= 0)
	{
		return false;
	}

	UWorld* World = Package->GetWorld();
	UPropsSubsystem* Props = World && World->GetGameInstance()
		? World->GetGameInstance()->GetSubsystem<UPropsSubsystem>()
		: nullptr;
	float HealthDelta = 0.0f;
	float HungerDelta = 0.0f;
	float ThirstDelta = 0.0f;
	if (!Props || !Props->GetConsumableEffects(ItemData->ID, HealthDelta, HungerDelta, ThirstDelta))
	{
		return false;
	}

	Package->SetSelectedSurvivalSlotId(ItemData->SurvivalSlotId);
	Package->RequestConsumeItemStack(ItemData->SurvivalSlotId, 1);
	return true;
}

bool UPackageListViewWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	if (APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwningPlayerPawn()))
	{
		if (UPackageComponent* Package = Player->GetPackageComponent())
		{
			if (UPackageItemWidget* PackageItemWidget = Cast<UPackageItemWidget>(InOperation->Payload))
			{
				if (PackageItemWidget->GetSceneItemActor())
				{
					Package->PickItemFromNear(PackageItemWidget->GetSceneItemActor());
				}
			}
			else if (USkinSlotWidget* SkinSlot = Cast<USkinSlotWidget>(InOperation->Payload))
			{
				Package->TakeOffToPackage(SkinSlot->GetSkinType());
			}
			else if (Cast<UWeaponFrameWidget>(InOperation->Payload))
			{
				Package->UnEquipWeaponToPackage();
			}
		}
	}
	return true;
}

void UPackageListViewWidget::OnAddItemActorToPackage(int32 Key, int32 ID)
{
	RefreshItems();
}

void UPackageListViewWidget::OnRemoveItemActorFromPackage(int32 Key, int32 ID)
{
	RefreshItems();
}

void UPackageListViewWidget::RefreshItems()
{
	if (!MyListView)
	{
		return;
	}

	APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwningPlayerPawn());
	UPackageComponent* Package = Player ? Player->GetPackageComponent() : nullptr;
	if (!Package)
	{
		return;
	}

	MyListView->ClearListItems();
	TArray<int32> Keys;
	Package->GetPackageItems().GetKeys(Keys);
	Keys.Sort();
	for (const int32 Key : Keys)
	{
		UPackageItemData* Data = NewObject<UPackageItemData>(this);
		if (!Data)
		{
			continue;
		}

		Data->Key = Key;
		Data->ID = Package->GetPackageItems()[Key];
		FItemStack SurvivalStack;
		if (Package->GetSurvivalStackForPackageKey(Key, SurvivalStack))
		{
			Data->bIsSurvivalStack = true;
			Data->SurvivalSlotId = SurvivalStack.SlotId;
			Data->Quantity = SurvivalStack.Quantity;
		}
		MyListView->AddItem(Data);
	}
}

void UPackageListViewWidget::UpdateSurvivalSelectionHighlight(int32 SelectedSlotId)
{
	if (!MyListView)
	{
		return;
	}

	for (UUserWidget* EntryWidget : MyListView->GetDisplayedEntryWidgets())
	{
		if (UPackageItemWidget* PackageItemWidget = Cast<UPackageItemWidget>(EntryWidget))
		{
			PackageItemWidget->UpdateSelectionHighlight(SelectedSlotId);
		}
	}
}
