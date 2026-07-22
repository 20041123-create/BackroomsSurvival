// Fill out your copyright notice in the Description page of Project Settings.


#include "HallHUD.h"

#include "LegoGame/GamePlay/GameMenu/Hall/HallUserWidget.h"

void AHallHUD::BeginPlay()
{
	Super::BeginPlay();
	
	//创建页面
	TSubclassOf<UHallUserWidget> WidgetClass = LoadClass<UHallUserWidget>(nullptr, TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/LegoGame/UMG/Hall/WBP_Hall.WBP_Hall_C'"));
	HallUserWidget = CreateWidget<UHallUserWidget>(GetOwningPlayerController(), WidgetClass);
	if (HallUserWidget)
	{
		HallUserWidget->AddToViewport();
		GetOwningPlayerController()->bShowMouseCursor = true;
	}
}
