// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RegisterUserWidget.generated.h"

class UEditableTextBox;
class UTextBlock;
/**
 * 
 */
class UButton;
UCLASS()
class LEGOGAME_API URegisterUserWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	//当父类只有一个构造函数，并且构造函数带参数，子类继承时，如果想要重写构造函数，则必须显式调用父类带参构造
	URegisterUserWidget(const FObjectInitializer& ObjectInitializer);
	
protected:
	
	virtual void NativeOnInitialized() override;
	
	UFUNCTION()
	void OnSendCodeButtonClicked();
	UFUNCTION()
	void OnTimerBegin();
	
	void UpdateRemainCodeDownTime();
	
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> SendCodeButton;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> SendCodeTextBlock;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UEditableTextBox> MailTextBox;
	
	UPROPERTY(EditAnywhere)
	int32 SendCodeDownTime;
	int32 RemainCodeDownTime;
};
