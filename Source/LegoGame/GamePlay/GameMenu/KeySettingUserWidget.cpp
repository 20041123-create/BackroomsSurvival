// Fill out your copyright notice in the Description page of Project Settings.


#include "KeySettingUserWidget.h"

#include "KeyInfoItem.h"
#include "Components/ScrollBox.h"
#include "Kismet/GameplayStatics.h"
#include "LegoGame/LegoGame.h"
#include "LegoGame/SaveGame/CustomKeySaveGame.h"


void UKeySettingUserWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	//加载数据表格
	DT_KeyMapping = LoadObject<UDataTable>(this,TEXT("/Script/Engine.DataTable'/Game/LegoGame/Data/DT_KeyMapping.DT_KeyMapping'"));
	//读取存档，检查存档是否存在
	if (UGameplayStatics::DoesSaveGameExist(CUSTOM_KEY_SLOT,0))
	{
		CustomKeySaveGame = Cast<UCustomKeySaveGame>(UGameplayStatics::LoadGameFromSlot(CUSTOM_KEY_SLOT,0));
	}
	if (DT_KeyMapping)
	{
		//解读表格数据
		//传常引用减少拷贝
		//第一种读取：只读取条目数据不读取行名称
		// TArray<FKsyInfoHeader*> AllKeyInfos;
		// DT_KeyMapping->GetAllRows<FKsyInfoHeader>(TEXT("读取表格错误"),AllKeyInfos);
		// TSubclassOf<UKeyInfoItem> WidgetClass = LoadClass<UKeyInfoItem>(nullptr,TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/LegoGame/UMG/GameMenu/WBP_KeyInfoItem.WBP_KeyInfoItem_C'"));
		// for (auto Item : AllKeyInfos)
		// {
		// 	//UE_LOG(LogTemp, Warning, TEXT("%s"), *Item->KeyDescription.ToString());
		// 	//没读取一个表格数据就创建一个UI添加到面板中
		// 	UKeyInfoItem* KeyInfoItemWidget = CreateWidget<UKeyInfoItem>(GetOwningPlayer(),WidgetClass);
		// 	KeyInfoItemWidget->InitPanel(Item);
		// 	//添加到滚动容器中
		// 	MyScrollBox->AddChild(KeyInfoItemWidget);
		// }
		
		for (auto KV : DT_KeyMapping->GetRowMap())
		{
			//指针的类型主要决定了指向空间的解释方式，uint8*不维护指针解释方式
			//要使用强制类型转换，需要考虑合理性
			FKsyInfoHeader* KeyInfoHeader = reinterpret_cast<FKsyInfoHeader*>(KV.Value);
			if (KeyInfoHeader->KeyDescription.IsEmpty())//没有描述说明不需要自定义
			{
				continue;
			}
			TSubclassOf<UKeyInfoItem> WidgetClass = LoadClass<UKeyInfoItem>(nullptr,TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/LegoGame/UMG/GameMenu/WBP_KeyInfoItem.WBP_KeyInfoItem_C'"));
			UKeyInfoItem* KeyInfoItemWidget = CreateWidget<UKeyInfoItem>(GetOwningPlayer(),WidgetClass);
			
			//通过行名称来检查是否有对应的存档按键
			FKey Key = GetCustomKey(KV.Key);
			if (!Key.IsValid())//如果按键无效使用表格中的，否则使用存档中的
			{
				Key = KeyInfoHeader->Key;
			}
			KeyInfoItemWidget->InitPanel(KV.Key,KeyInfoHeader->KeyDescription,Key);
			MyScrollBox->AddChild(KeyInfoItemWidget);
		}
	}
}

FKey UKeySettingUserWidget::GetCustomKey(FName KeyEventName)
{
	//你给我一个行名称，我来检查是否有存档，如果有就返回存档内自定义按键
	FKey Key;
	if (CustomKeySaveGame && CustomKeySaveGame->CustomKey.Contains(KeyEventName))
	{
		Key = CustomKeySaveGame->CustomKey[KeyEventName];
	}
	return Key;
}

void UKeySettingUserWidget::ResetAllKeys()
{
	//重置所有按键
	//删掉所有存档
	UGameplayStatics::DeleteGameInSlot(CUSTOM_KEY_SLOT,0);
	//变更UI
	for (int32 i = 0;i<MyScrollBox->GetChildrenCount();i++)
	{
		if (UKeyInfoItem* InfoItemWidget = Cast<UKeyInfoItem>(MyScrollBox->GetChildAt(i)))
		{
			if (DT_KeyMapping->GetRowMap().Contains(InfoItemWidget->GetKeyEventName()))
			{
				FKsyInfoHeader* KeyInfoHeader = reinterpret_cast<FKsyInfoHeader*> (DT_KeyMapping->GetRowMap()[InfoItemWidget->GetKeyEventName()]);
				InfoItemWidget->ResetKey(KeyInfoHeader->Key);
			}
		}
	}
}

void UKeySettingUserWidget::SaveCustomKey(FName KeyEventName, FKey NewKey)
{
	if (!CustomKeySaveGame)//没有存档创造存档
	{
		CustomKeySaveGame = Cast<UCustomKeySaveGame>(UGameplayStatics::CreateSaveGameObject(UCustomKeySaveGame::StaticClass()));
	}
	//检查是否已经存档（记录过对应按键）
	if (CustomKeySaveGame->CustomKey.Contains(KeyEventName))//Contains函数的意图：检查是否存在给定的键
	{
		CustomKeySaveGame->CustomKey[KeyEventName] = NewKey;
	}
	else
	{
		CustomKeySaveGame->CustomKey.Add(KeyEventName,NewKey);
	}
	//存到磁盘
	UGameplayStatics::SaveGameToSlot(CustomKeySaveGame,CUSTOM_KEY_SLOT,0);
}
