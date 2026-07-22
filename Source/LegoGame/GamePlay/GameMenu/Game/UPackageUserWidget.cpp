// Fill out your copyright notice in the Description page of Project Settings.


#include "UPackageUserWidget.h"

#include "PackageItemWidget.h"
#include "PackageListViewWidget.h"
#include "SkinSlotWidget.h"
#include "WeaponFrameWidget.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"


#include "LegoGame/Components/PackageComponent.h"
#include "LegoGame/Player/PlayerCharacter.h"
#include "LegoGame/Player/PlayerModelActor.h"
#include "LegoGame/Scene/SceneItemActor.h"

void UUPackageUserWidget::RemoveFromParent()
{
	Super::RemoveFromParent();//!HasAnyFlags(RF_ClassDefaultObject)检查是否为类默认对象(CDO)
	if (!HasAnyFlags(RF_ClassDefaultObject)&&NearBox)//一定要做检查，因为remove函数会在没有启动项目的时候编译器就会调用，但是调用的时候组件是没有绑定的，就会出现崩溃
	{
		//移除容器内的子内容
		NearBox->ClearChildren();
	}
	//解除代理绑定
	if (APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwningPlayerPawn()))
	{
		if (Player->GetPackageComponent())
		{
			Player->GetPackageComponent()->OnAddNearItemActor.RemoveAll(this);
			Player->GetPackageComponent()->OnRemoveNearItemActor.RemoveAll(this);
			
			Player->GetPackageComponent()->OnAddItemToPackage.RemoveAll(PackageListViewWidget);
			Player->GetPackageComponent()->OnRemoveItemFromPackage.RemoveAll(PackageListViewWidget);
			
			Player->GetPackageComponent()->OnPutOnSkin.RemoveAll(this);
			Player->GetPackageComponent()->OnTakeOffSkin.RemoveAll(this);
			
			Player->GetPackageComponent()->OnEquipWeapon.RemoveAll(this);
			Player->GetPackageComponent()->OnUnEquipWeapon.RemoveAll(this);
		}
	}
	if (PlayerModelActor)
	{
		PlayerModelActor->SetActorHiddenInGame(true);
	}
}


void UUPackageUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	
	//更新背包UI列表数据，主要是附近的数据
	if (APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwningPlayerPawn()))
	{
		if (!PackageItemWidgetClass)
		{
			PackageItemWidgetClass = LoadClass<UPackageItemWidget>(this, TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/LegoGame/UMG/Game/WBP_PackageItem.WBP_PackageItem_C'"));
		}
		//NearBox->ClearChildren();
		for (auto Item : Player->GetPackageComponent()->GetNearItems())
		{
			if (IsValid(Item))
			{
				UPackageItemWidget* PackageItemWidget = CreateWidget<UPackageItemWidget>(GetOwningPlayer(), PackageItemWidgetClass);
				PackageItemWidget->InitPanel(Item);
				NearBox->AddChild(PackageItemWidget);
				//UE_LOG(LogTemp, Warning, TEXT("%s"),*Item->GetName());
			}
		}
		
		//绑定代理
		//FDelegateHandle hand = Player->GetPackageComponent()->OnAddNearItemActor.AddUObject(this,&ThisClass::OnAddNearItemActor);
		Player->GetPackageComponent()->OnAddNearItemActor.AddUObject(this,&ThisClass::OnAddNearItemActor);
		Player->GetPackageComponent()->OnRemoveNearItemActor.AddUObject(this,&ThisClass::OnRemoveNearItemActor);
		//指定性解绑
		//Player->GetPackageComponent()->OnAddNearItemActor.Remove(hand);
		//解除所有绑定
		//Player->GetPackageComponent()->OnAddNearItemActor.RemoveAll(this);
		Player->GetPackageComponent()->OnAddItemToPackage.AddUObject(PackageListViewWidget,&UPackageListViewWidget::OnAddItemActorToPackage);
		Player->GetPackageComponent()->OnRemoveItemFromPackage.AddUObject(PackageListViewWidget,&UPackageListViewWidget::OnRemoveItemActorFromPackage);
		
		Player->GetPackageComponent()->OnPutOnSkin.AddUObject(this,&ThisClass::OnPutOnSkin);
		Player->GetPackageComponent()->OnTakeOffSkin.AddUObject(this,&ThisClass::OnTakeOffSkin);
		
		Player->GetPackageComponent()->OnEquipWeapon.AddUObject(this,&ThisClass::OnEquipWeapon);
		Player->GetPackageComponent()->OnUnEquipWeapon.AddUObject(this,&ThisClass::OnUnEquipWeapon);
		
		//生成展示actor
		if (!PlayerModelActor)
		{
			TSubclassOf<APlayerModelActor> ActorClass = LoadClass<APlayerModelActor>(nullptr,TEXT("/Script/Engine.Blueprint'/Game/LegoGame/Blueprints/Player/BP_PlayerModel.BP_PlayerModel_C'"));
			PlayerModelActor = GetWorld()->SpawnActor<APlayerModelActor>(ActorClass,FVector(0,0,3000),FRotator(0,180,0));
			if (PlayerModelActor)
			{
				PlayerModelActor->SetBindPlayer(Player);
			}
		}
		if (PlayerModelActor)
		{
			PlayerModelActor->SetActorHiddenInGame(false);
		}
	}
	
}

void UUPackageUserWidget::OnAddNearItemActor(ASceneItemActor* SceneItemActor)
{
	UPackageItemWidget* PackageItemWidget = CreateWidget<UPackageItemWidget>(GetOwningPlayer(), PackageItemWidgetClass);
	PackageItemWidget->InitPanel(SceneItemActor);
	NearBox->AddChild(PackageItemWidget);
	
}

void UUPackageUserWidget::OnRemoveNearItemActor(ASceneItemActor* SceneItemActor)
{
	for (int32 i = 0;i<NearBox->GetChildrenCount();i++)
	{
		if (UPackageItemWidget* ItemWidget = Cast<UPackageItemWidget>(NearBox->GetChildAt(i)))
		{
			if (ItemWidget->GetSceneItemActor() == SceneItemActor)
			{
				ItemWidget->RemoveFromParent();
				break;
			}
		}
	}
	
}

bool UUPackageUserWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	if (APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwningPlayerPawn()))
	{
		if (Player->GetPackageComponent())
		{
			if (UPackageItemWidget* PackageItem = Cast<UPackageItemWidget>(InOperation->Payload))
			{
				if (!PackageItem->GetSceneItemActor())//没有关联场景道具说明是背包的
				{
					Player->GetPackageComponent()->RemoveItemFromPackageToScene(PackageItem->GetPackageKey());
				}
			}
			else if (USkinSlotWidget * SkinSlot = Cast<USkinSlotWidget>(InOperation->Payload))
			{
				Player->GetPackageComponent()->TakeOffToScene(SkinSlot->GetSkinType());
			}
			else if (Cast<UWeaponFrameWidget>(InOperation->Payload))
			{
				Player->GetPackageComponent()->UnEquipWeaponToScene();
			}
		}
	}
	return true;
}

void UUPackageUserWidget::OnPutOnSkin(ESkinType SkinType, int32 ID)
{
	for (int i = 0;i<SkinVerticalBox->GetChildrenCount();i++)
	{
		if (USkinSlotWidget* SkinSlotWidget = Cast<USkinSlotWidget>(SkinVerticalBox->GetChildAt(i)))
		{
			if (SkinSlotWidget->GetSkinType() == SkinType)
			{
				
				SkinSlotWidget->InitPanel(ID);
				break;
			}
		}
	}
	
}

void UUPackageUserWidget::OnTakeOffSkin(ESkinType SkinType, int32 ID)
{
	for (int i = 0;i<SkinVerticalBox->GetChildrenCount();i++)
	{
		if (USkinSlotWidget* SkinSlotWidget = Cast<USkinSlotWidget>(SkinVerticalBox->GetChildAt(i)))
		{
			if (SkinSlotWidget->GetSkinType() == SkinType)
			{
				
				SkinSlotWidget->InitPanel(-1);
				break;
			}
		}
	}
	
}

void UUPackageUserWidget::OnEquipWeapon(int32 ID)
{
	WeaponFrame->InitPanel(ID);
	
}

void UUPackageUserWidget::OnUnEquipWeapon(int32 ID)
{
	WeaponFrame->InitPanel(-1);
	
}


