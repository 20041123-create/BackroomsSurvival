// Fill out your copyright notice in the Description page of Project Settings.


#include "SkinSlotWidget.h"

#include "PackageItemWidget.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "LegoGame/Components/PackageComponent.h"
#include "LegoGame/Player/PlayerCharacter.h"
#include "LegoGame/Subsystem/PropsSubsystem.h"

void USkinSlotWidget::InitPanel(int32 ID)
{
	if (ID==-1)
	{
		SkinTextBlock->SetVisibility(ESlateVisibility::Visible);
		IconImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	const FPropsBase* Props = GetWorld()->GetGameInstance()->GetSubsystem<UPropsSubsystem>()->GetPropsById(ID);
	if (Props)
	{
		IconImage->SetVisibility(ESlateVisibility::Visible);
		SkinTextBlock->SetVisibility(ESlateVisibility::Hidden);
		IconImage->SetBrushFromTexture(Props->Icon);
	}
	
}

FReply USkinSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!IconImage->IsVisible())
	{
		return FReply::Unhandled();
	}
	return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent,this,EKeys::LeftMouseButton).NativeReply;
	
}

void USkinSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	//Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);
	OutOperation = UWidgetBlueprintLibrary::CreateDragDropOperation(UDragDropOperation::StaticClass());
	OutOperation->Payload = this;
	OutOperation->DefaultDragVisual = this;
	OutOperation->Pivot = EDragPivot::MouseDown;
}

bool USkinSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
                                   UDragDropOperation* InOperation)
{
	if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn()))
	{
		if (PlayerCharacter->GetPackageComponent())
		{
			//检查拖拽的是否是附近的或者是背包列表的UI控件
			if (UPackageItemWidget* ItemWidget = Cast<UPackageItemWidget>(InOperation->Payload))
			{
				//判断是附近的还是背包的
				if (ItemWidget->GetSceneItemActor())
				{
					PlayerCharacter->GetPackageComponent()->PutOnSkinFromNear(ItemWidget->GetSceneItemActor(),SkinType);
				}
				else
				{
					PlayerCharacter->GetPackageComponent()->PutOnSkinFromPackage(ItemWidget->GetPackageKey(),SkinType);
				}
			}
		}
	}
	return true;
}

void USkinSlotWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	//根据当前的部件类别更新文本内容
	FText SkinText = NSLOCTEXT("ui","dikcs","空");
	switch (SkinType)
	{
	case ESkinType::EST_Cap:
		SkinText = NSLOCTEXT("ui","d1ikcs","帽");
		break;
	case ESkinType::EST_Beard:
		SkinText = NSLOCTEXT("ui","d2ikcs","胡");
		break;
	case ESkinType::EST_Clothes:
		SkinText = NSLOCTEXT("ui","d3ikcs","衣");
		break;
	case ESkinType::EST_Glasses:
		SkinText = NSLOCTEXT("ui","d4ikcs","镜");
		break;
	case ESkinType::EST_Hair:
		SkinText = NSLOCTEXT("ui","d5ikcs","发");
		break;
	case ESkinType::EST_Helmet:
		SkinText = NSLOCTEXT("ui","d6ikcs","盔");
		break;
	case ESkinType::EST_Masker:
		SkinText = NSLOCTEXT("ui","d7ikcs","面");
		break;
	case ESkinType::EST_Package:
		SkinText = NSLOCTEXT("ui","d8ikcs","包");
		break;
	case ESkinType::EST_None:
		break;
	}
	SkinTextBlock->SetText(SkinText);
	
}
