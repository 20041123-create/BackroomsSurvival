// Fill out your copyright notice in the Description page of Project Settings.


#include "PackageListViewWidget.h"

#include "PackageItemWidget.h"
#include "SkinSlotWidget.h"
#include "WeaponFrameWidget.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/ListView.h"

#include "LegoGame/Components/PackageComponent.h"
#include "LegoGame/Data/PackageItemData.h"
#include "LegoGame/Player/PlayerCharacter.h"


//此函数需要返回一个bool值，目的是告诉引擎管理器，你是否要消耗掉当前的松手，否则继续向下传递
bool UPackageListViewWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
                                          UDragDropOperation* InOperation)
{
	//拖拽道具松手响应逻辑
	if (APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwningPlayerPawn()))
	{
		if (Player->GetPackageComponent())
		{
			//拾取道具到背包
			//先检查拖拽的是什m
			if (UPackageItemWidget* PackageItemWidget = Cast<UPackageItemWidget>(InOperation->Payload))//通过InOperation中的Payload来判断拖拽的是什么
			{
				if (PackageItemWidget->GetSceneItemActor())
				{
					Player->GetPackageComponent()->PickItemFromNear(PackageItemWidget->GetSceneItemActor());
				}
			}
			else if (USkinSlotWidget * SkinSlot = Cast<USkinSlotWidget>(InOperation->Payload))
			{
				Player->GetPackageComponent()->TakeOffToPackage(SkinSlot->GetSkinType());
			}
			else if (Cast<UWeaponFrameWidget>(InOperation->Payload))
			{
				Player->GetPackageComponent()->UnEquipWeaponToPackage();
			}
		}
	}
	return true;
}


void UPackageListViewWidget::OnAddItemActorToPackage(int32 Key, int32 ID)
{
	if (!MyListView)
	{
		UE_LOG(LogTemp, Warning, TEXT("okokok"));
		return;
	}
	if (UPackageItemData* Data = NewObject<UPackageItemData>(this))
	{
		// Data->ItemKey = Key;
		// Data->ItemID = ID;
		Data->Key = Key;
		Data->ID = ID;
		MyListView->AddItem(Data);
		//UE_LOG(LogTemp, Warning, TEXT("okokok"));// 此时 Data 的身份是合法的，这里就不会崩了
	}
}

void UPackageListViewWidget::OnRemoveItemActorFromPackage(int32 Key, int32 ID)
{
	//通知一个道具被扔掉
	for (int32 i = 0;i<MyListView->GetNumItems();++i)
	{
		if (UPackageItemData* Data = Cast<UPackageItemData>(MyListView->GetItemAt(i)))
		{
			if (Data->Key == Key)
			{
				MyListView->RemoveItem(Data);
				break;
			}
		}
	}
}