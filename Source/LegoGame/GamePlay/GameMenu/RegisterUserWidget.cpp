// Fill out your copyright notice in the Description page of Project Settings.


#include "RegisterUserWidget.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"

URegisterUserWidget::URegisterUserWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)//保证正常出生
{
	SendCodeDownTime = 5;
}

void URegisterUserWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	//绑定按键通知
	SendCodeButton->OnClicked.AddDynamic(this,&ThisClass::OnSendCodeButtonClicked);
}

void URegisterUserWidget::OnSendCodeButtonClicked()
{
	//检查输入的文本是否是邮箱地址
	FRegexPattern const Pattern(TEXT("^[a-zA-Z0-9_.+-]+@[a-zA-Z0-9-]+\\.[a-zA-Z0-9-.]+$"));//正则规则
	FRegexMatcher Matcher(Pattern,MailTextBox->GetText().ToString());//构建匹配检查器
	//检查字符串是否符合正则表达式规则
	if (!Matcher.FindNext())
	{
		UE_LOG(LogTemp, Warning, TEXT("错误"));
		return;
	}
	
	//设置按钮为不可用
	SendCodeButton->SetIsEnabled(false);
	RemainCodeDownTime = SendCodeDownTime;
	//显示倒计时
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle,this,&ThisClass::OnTimerBegin,1);
	UpdateRemainCodeDownTime();
}

void URegisterUserWidget::OnTimerBegin()
{
	if (--RemainCodeDownTime>0)
	{
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle,this,&ThisClass::OnTimerBegin,1);
		UpdateRemainCodeDownTime();
	}
	else
	{
		SendCodeButton->SetIsEnabled(true);
		UpdateRemainCodeDownTime();
	}
}

void URegisterUserWidget::UpdateRemainCodeDownTime()
{
	if (RemainCodeDownTime>0)
	{
		SendCodeTextBlock->SetText(FText::Format(NSLOCTEXT("UI","CE123","发送（{0}）"),FText::AsNumber(RemainCodeDownTime)));
	}
	else
	{
		SendCodeTextBlock->SetText(NSLOCTEXT("UI","CE124","发送"));
	}
}


