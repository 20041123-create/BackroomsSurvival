// Fill out your copyright notice in the Description page of Project Settings.


#include "LgHUD.h"

#include "Engine/Canvas.h"
#include "LegoGame/GamePlay/GameMenu/Game/GameFetureUserWidget.h"
#include "LegoGame/GamePlay/GameMenu/Game/UPackageUserWidget.h"
#include "LegoGame/Player/PlayerCharacter.h"

void ALgHUD::TogglePackageUI()
{
	//UE_LOG(LogTemp, Warning, TEXT("xxx"));
	if (!PackageUserWidget)
	{
		TSubclassOf<UUPackageUserWidget> WidgetClass = LoadClass<UUPackageUserWidget>(nullptr,TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/LegoGame/UMG/Game/WBP_Package.WBP_Package_C'"));
		PackageUserWidget = CreateWidget<UUPackageUserWidget>(GetOwningPlayerController(),WidgetClass);
	}
	if (PackageUserWidget)
	{
		//显示到视口中就移除，不在视口中就加入
		if (PackageUserWidget->IsInViewport())
		{
			PackageUserWidget->RemoveFromParent();
			//显示鼠标光标
			GetOwningPlayerController()->bShowMouseCursor = false;
		}
		else
		{
			PackageUserWidget->AddToViewport();
			//显示鼠标光标
			GetOwningPlayerController()->bShowMouseCursor = true;
		}
		
	}
}

void ALgHUD::BeginPlay()
{
	Super::BeginPlay();
	TSubclassOf<UGameFetureUserWidget> WidgetClass = LoadClass<UGameFetureUserWidget>(nullptr,TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/LegoGame/UMG/Game/WBP_GameFeture.WBP_GameFeture_C'"));
	GameFetureUserWidget = CreateWidget<UGameFetureUserWidget>(GetOwningPlayerController(),WidgetClass);
	if (GameFetureUserWidget)
	{
		GameFetureUserWidget->AddToViewport();
	}
}

void ALgHUD::DrawHUD()//每帧都会走
{
	Super::DrawHUD();
	//绘制准心
	if (!PlayerCharacter)
	{
		PlayerCharacter = Cast<APlayerCharacter>(GetOwningPawn());
		return;
	}
	//绘制
	if (PlayerCharacter->IsIronSight())
	{
		//准心的长度
		float AimSignLen = 8.f;
		//准心到屏幕中心的距离
		float AimSignOff = 5.f;
		//屏幕中心坐标
		float CenterX = Canvas->ClipX/2;
		float CenterY = Canvas->ClipY/2;
		
		//绘制准心 上下
		DrawLine(CenterX,CenterY-AimSignLen-AimSignOff,CenterX,CenterY-AimSignOff,FLinearColor::Green,2);
		DrawLine(CenterX,CenterY+AimSignLen+AimSignOff,CenterX,CenterY+AimSignOff,FLinearColor::Green,2);
		//左右
		DrawLine(CenterX-AimSignOff-AimSignLen,CenterY,CenterX-AimSignOff,CenterY,FLinearColor::Green,2);
		DrawLine(CenterX+AimSignOff+AimSignLen,CenterY,CenterX+AimSignOff,CenterY,FLinearColor::Green,2);
		
	}
	
}
