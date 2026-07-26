// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMenuHUD.h"

#include "SettingUserWidget.h"
#include "LegoGame/GameMenu/GameMenuUserWidget.h"
#include "LoginUserWidget.h"
#include "RegisterUserWidget.h"

void AGameMenuHUD::BeginPlay()
{
	Super::BeginPlay();
	ShowGameMenuUI();
	
}

void AGameMenuHUD::ShowGameMenuUI()
{
	//显示UI
	//手动加载UMG类
	//参数说明：1.Outer:加载的资源属于谁,这样操作方便后期的内存管理
	//当一个对象被加载到内存中，引擎会检查你内部数据对象，归属于我的数据对象会随之被释放（并不是绝对，当对象被其他人持有会跳过释放）
	//当Outer填入的nullptr,则这个对象会放置在临时空间，临时空间随时可能被引擎回收
	if (!GameMenuUserWidget)
	{
		TSubclassOf<UGameMenuUserWidget>  WidgetClass = LoadClass<UGameMenuUserWidget>(nullptr,TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/LegoGame/UMG/GameMenu/WBP_GameMenu.WBP_GameMenu_C'"));
		//创建对象 借助全集函数CreateWidget
		GameMenuUserWidget = CreateWidget<UGameMenuUserWidget>(GetOwningPlayerController(),WidgetClass);
	}
	
	if (GameMenuUserWidget)
	{
		GameMenuUserWidget->AddToViewport();//添加到视口
		//显示鼠标光标
		GetOwningPlayerController()->bShowMouseCursor = true;
	}
	
}

void AGameMenuHUD::ShowSettingUI()
{
	if (!SettingUserWidget)//为空加载示例
	{
		TSubclassOf<UGameMenuUserWidget>  WidgetClass = LoadClass<USettingUserWidget>(nullptr,TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/LegoGame/UMG/GameMenu/WBP_Setting.WBP_Setting_C'"));
		SettingUserWidget = CreateWidget<USettingUserWidget>(GetOwningPlayerController(),WidgetClass);
	}
	if (SettingUserWidget)
	{
		SettingUserWidget->AddToViewport();
	}
}

void AGameMenuHUD::ShowLoginUI()
{
	if (!LoginUserWidget)//为空加载示例
	{
		TSubclassOf<ULoginUserWidget>  WidgetClass = LoadClass<ULoginUserWidget>(nullptr,TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/LegoGame/UMG/GameMenu/WBP_Login.WBP_Login_C'"));
		LoginUserWidget = CreateWidget<ULoginUserWidget>(GetOwningPlayerController(),WidgetClass);
	}
	if (LoginUserWidget)
	{
		LoginUserWidget->AddToViewport();
	}
}

void AGameMenuHUD::ShowRegisterUI()
{
	if (!RegisterUserWidget)//为空加载示例
	{
		TSubclassOf<URegisterUserWidget>  WidgetClass = LoadClass<URegisterUserWidget>(nullptr,TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/LegoGame/UMG/GameMenu/WBP_Register.WBP_Register_C'"));
		RegisterUserWidget = CreateWidget<URegisterUserWidget>(GetOwningPlayerController(),WidgetClass);
	}
	if (RegisterUserWidget)
	{
		RegisterUserWidget->AddToViewport();
	}
}

void AGameMenuHUD::ShowRoomListUI()
{
	if (!RoomListWidget)
	{
		TSubclassOf<UUserWidget> WidgetClass = LoadClass<UUserWidget>(nullptr,TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/LegoGame/UMG/GameMenu/WBP_RoomList.WBP_RoomList_C'"));
		RoomListWidget = CreateWidget<UUserWidget>(GetOwningPlayerController(),WidgetClass);
	}
	if (RoomListWidget)
	{
		RoomListWidget->AddToViewport();
	}
}

void AGameMenuHUD::ShowCreateRoomUI()
{
	if (!CreateRoomWidget)
	{
		TSubclassOf<UUserWidget> WidgetClass = LoadClass<UUserWidget>(nullptr,TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/LegoGame/UMG/GameMenu/WBP_CreateRoom.WBP_CreateRoom_C'"));
		CreateRoomWidget = CreateWidget<UUserWidget>(GetOwningPlayerController(),WidgetClass);
	}
	if (CreateRoomWidget)
	{
		CreateRoomWidget->AddToViewport();
	}
}

void AGameMenuHUD::ShowWaitingUI()
{
	if (!WaitingWidget)
	{
		TSubclassOf<UUserWidget> WidgetClass = LoadClass<UUserWidget>(nullptr,TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/LegoGame/UMG/GameMenu/WBP_Waiting.WBP_Waiting_C'"));
		WaitingWidget = CreateWidget<UUserWidget>(GetOwningPlayerController(),WidgetClass);
	}
	if (WaitingWidget)
	{
		WaitingWidget->AddToViewport();
	}
}


