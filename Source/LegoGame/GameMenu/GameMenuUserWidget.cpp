// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMenuUserWidget.h"

#include "Components/TextBlock.h"

//类似于BeginPlay
void UGameMenuUserWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	//通过控件称获得控件
	//由于父类不能转为子类。所以使用Cast进行推导转换，成立返回有效指针，不成立返回空
	//MyTextBlock = Cast<UTextBlock>(GetWidgetFromName("TextBlock_0"));
	
}
//如果你想绑定按钮、初始化那些永远不会变的指针：请用 NativeOnInitialized。整个生命周期仅 1 次
//如果你想在 UI 弹出时播放动画、或者同步最新的游戏状态：请用 NativeConstruct。严禁在此绑定（会重复）只要重新显示就会触发
void UGameMenuUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	PlayAnimation(MenuAnimation,0,1,EUMGSequencePlayMode::Forward,2.f);
}


// void UGameMenuUserWidget::CallFunction()
// {
// 	//UE_LOG(LogTemp, Warning, TEXT("UG"));
// 	//想办法取到蓝图控件
// 	// if (GetTextBlock())
// 	// {
// 	// 	num++;
// 	// 	//将int32转换为FText
// 	// 	GetTextBlock()->SetText(FText::AsNumber(num));
// 	// 	
// 	// }
// 	 if (MyTextBlock)
// 	 {
// 	 	num++;
// 	 	//将int32转换为FText
// 	 	MyTextBlock->SetText(FText::AsNumber(num));
// 	 }
// 	
// }

