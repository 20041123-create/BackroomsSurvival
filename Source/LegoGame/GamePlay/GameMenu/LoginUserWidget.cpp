// Fill out your copyright notice in the Description page of Project Settings.


#include "LoginUserWidget.h"

#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
#include "Kismet/GameplayStatics.h"
#include "LegoGame/SaveGame/AccountSaveGame.h"

#define PASSWORD_SLOT TEXT("UserPassword")

void ULoginUserWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	ShowPasswordButten->OnPressed.AddDynamic(this,&ThisClass::OnShowPasswordPressed);
	ShowPasswordButten->OnReleased.AddDynamic(this,&ThisClass::OnHidePasswordReleased);
	
	//检查是否保存过密码，如果有则显示到UI中
	if (UGameplayStatics::DoesSaveGameExist(PASSWORD_SLOT,0))
	{
		AccountSaveGame = Cast<UAccountSaveGame>(UGameplayStatics::LoadGameFromSlot(PASSWORD_SLOT,0));
		AccountTextBox->SetText(FText::FromString(AccountSaveGame->AccountString));//FString转到FText，借助工具函数
		PasswordTextBox->SetText(FText::FromString(AccountSaveGame->PasswordString));
		PasswordTextBox->SetIsPassword(true);
		PasswordCheckBox->SetIsChecked(true);
	}
}

void ULoginUserWidget::OnShowPasswordPressed()
{
	PasswordTextBox->SetIsPassword(false);
	
}

void ULoginUserWidget::OnHidePasswordReleased()
{
	PasswordTextBox->SetIsPassword(true);
	
}

void ULoginUserWidget::LoginGame()
{
	if (PasswordCheckBox->IsChecked())
	{
		//创建一个存档对象
		if (!AccountSaveGame)
		{
			AccountSaveGame = Cast<UAccountSaveGame>(UGameplayStatics::CreateSaveGameObject(UAccountSaveGame::StaticClass()));
		}
		AccountSaveGame->AccountString = AccountTextBox->GetText().ToString();
		AccountSaveGame->PasswordString = PasswordTextBox->GetText().ToString();
		//保存到磁盘
		UGameplayStatics::SaveGameToSlot(AccountSaveGame,PASSWORD_SLOT,0);
	}
	else
	{
		//删掉存档
		UGameplayStatics::DeleteGameInSlot(PASSWORD_SLOT,0);
	}
}


#undef PASSWORD_SLOT 