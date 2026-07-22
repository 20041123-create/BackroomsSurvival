// Fill out your copyright notice in the Description page of Project Settings.


#include "KeyInfoItem.h"

#include "GameMenuHUD.h"
#include "KeySettingUserWidget.h"
#include "SettingUserWidget.h"
#include "Components/InputKeySelector.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"


void UKeyInfoItem::InitPanel(FName OutKeyEventName,FText KeyDescribe,FKey Key)
{
	//更新页面内容
	TextBlock->SetText(KeyDescribe);
	InputKeySelector->SetSelectedKey(Key);
	
	CurrentKey = Key;
	KeyEventName = OutKeyEventName;
}

void UKeyInfoItem::ResetKey(FKey DefaultKey)
{
	//将UI显示的按键强制显示成Default
	if (CurrentKey == DefaultKey)
	{
		return;
	}
	CurrentKey = DefaultKey;//要先执行此代码，因为后面要调用SetSelectKey出现回调通知，会保存按键信息，主要防止继续存储信
	InputKeySelector->SetSelectedKey(DefaultKey);
}

void UKeyInfoItem::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	InputKeySelector->OnKeySelected.AddDynamic(this,&ThisClass::OnKeySelected);
	
}

void UKeyInfoItem::OnKeySelected(FInputChord SelectKey)
{
	//输入按键此函数执行
	//UE_LOG(LogTemp, Warning, TEXT("%s"),*SelectKey.Key.ToString());
	//将用户输入的新的按键进行绑定存储
	if (CurrentKey == SelectKey.Key)
	{
		return;
	}
	//跟其他按键查重，如果重复则为不要继续使用按键
	
	for (int32 i = 0;i<GetParent()->GetChildrenCount();i++)
	{
		if (UKeyInfoItem* KeyInfoItemWidget = Cast<UKeyInfoItem>(GetParent()->GetChildAt(i)))
		{
			if (KeyInfoItemWidget->CurrentKey == SelectKey.Key)//说明拾取的控件已经被其他人占用
			{
				InputKeySelector->SetSelectedKey(CurrentKey);
				return;
			}
		}
	}
	
	if (AGameMenuHUD* Hud = Cast<AGameMenuHUD>(GetOwningPlayer()->GetHUD()))
	{
		Hud->GetSettingUserWidget()->GetKeySettingUserWidget()->SaveCustomKey(KeyEventName,SelectKey.Key);
	}
	CurrentKey = SelectKey.Key;
	
}
