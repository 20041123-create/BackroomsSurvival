// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMenuGameMode.h"

#include <string>

#include "GameMenuHUD.h"

AGameMenuGameMode::AGameMenuGameMode()
{
	HUDClass = AGameMenuHUD::StaticClass();
}

void AGameMenuGameMode::TestFunc()
{
	//构建一个Ftext需要一个构建宏来完成构建
	FText t1 = NSLOCTEXT("UEUI","slcfstd::to_string(2","恨你!");
	//输出
	UE_LOG(LogTemp, Warning, TEXT("%s"),*t1.ToString());
	//输出到屏幕
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,t1.ToString());
}
