// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameMenuUserWidget.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class LEGOGAME_API UGameMenuUserWidget : public UUserWidget
{
	GENERATED_BODY()
	protected:
	virtual void NativeOnInitialized() override;
	
	virtual void NativeConstruct() override;
	
	
	// protected:
	// UFUNCTION(BlueprintCallable)
	// void CallFunction();
	//
	// //BlueprintImplementableEvent模拟父类里面声明一个纯虚函数
	// //不是纯虚函数，你不需要（也不能）在 .cpp 文件中为它写函数体。
	// //但带有这种宏的 C++ 类依然可以被实例化。
	// //使用情景：BlueprintImplementableEvent 的核心用处是：在 C++ 中定义“时机”或“接口”，在蓝图中实现“细节”或“效果”。
	// //它非常适合处理那些视觉表现、声音特效、UI 更新或特定关卡逻辑，因为这些内容在蓝图中修改起来比 C++ 快得多。
	// UFUNCTION(BlueprintImplementableEvent)
	// UTextBlock* GetTextBlock();
	//
	// UPROPERTY(BlueprintReadWrite,meta=(BindWidget));//强制绑定，推荐
	// TObjectPtr<UTextBlock> MyTextBlock;
	//
	// int32 num;
protected:
	//Transient：防止序列化（内存优化）不保存到磁盘,强制垃圾回收逻辑
	UPROPERTY(meta = (BindWidgetAnim),Transient)
	TObjectPtr<UWidgetAnimation> MenuAnimation;
};
