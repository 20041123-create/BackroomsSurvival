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
	//当附近道具关联的UI面板时，点击鼠标右键直接拾取到背包中
	if (SceneItemActor && InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		//拾取道具
		if (APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwningPlayerPawn()))
		{
			if (Player->GetPackageComponent())
			{
				Player->GetPackageComponent()->PickItemFromNear(SceneItemActor);
				return FReply::Handled();
			}
		}
	}
	
	//此函数当鼠标放在控件上点击时调用
	//FReply是用来告知UI按键事件系统，我当前控件是否要响应这个按键，如果我返回的是FReplay::Handled(),则表明我需要响应这个按键事件，其他人没有响应权限
	//FReplay::Unhandled(),则表明我不处理这个按键，其他人可以处理
	//开启拖拽检测
	return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent,this,EKeys::LeftMouseButton).NativeReply;
	
}

void UPackageItemWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);
	//创建拖拽代理
	OutOperation = UWidgetBlueprintLibrary::CreateDragDropOperation(UDragDropOperation::StaticClass());
	//设置拖拽数据信息、
	//记录拖拽的是谁
	OutOperation->Payload = this;
	//设置拖拽的虚拟体
	//TODO 这里有一个BUG,需要创建一个虚拟体，暂时使用this替代
	OutOperation->DefaultDragVisual = CopySelf();
	//设置拖拽过程中虚拟体对齐鼠标的位置
	OutOperation->Pivot = EDragPivot::MouseDown;
}


void UPackageItemWidget::InitPanel(TObjectPtr<ASceneItemActor> InSceneItemActor)
{
	if (IsValid(InSceneItemActor))//IsValid检查是否存在于内存中，还是已经被标记释放了
	{
		InitPanel(InSceneItemActor->GetID());
		SceneItemActor = InSceneItemActor;
	}
	
}

void UPackageItemWidget::InitPanel(int32 ID)
{
	if (const FPropsBase* PropsBase = GetWorld()->GetGameInstance()->GetSubsystem<UPropsSubsystem>()->GetPropsById(ID))
	{
			//更新名称
		NameTextBlock->SetText(PropsBase->Name);
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

void UPackageItemWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	if (UPackageItemData* Data = Cast<UPackageItemData>(ListItemObject))
	{
		InitPanel(Data->ID);
		PackageKey = Data->Key;
	}
	
}


