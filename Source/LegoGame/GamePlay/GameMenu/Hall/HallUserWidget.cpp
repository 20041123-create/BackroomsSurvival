// Fill out your copyright notice in the Description page of Project Settings.


#include "HallUserWidget.h"

#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "LegoGame/LegoGame.h"
#include "LegoGame/GamePlay/Hall/HallGameState.h"
#include "LegoGame/GamePlay/GameMenu/Hall/HallUserInfoWidget.h"
#include "LegoGame/GamePlay/Hall/HallGameMode.h"
#include "LegoGame/GamePlay/Hall/HallPlayerController.h"
#include "LegoGame/GamePlay/Hall/HallPlayerState.h"

UScrollBox* UHallUserWidget::GetScrollBox(ETeamType TeamType)
{
	if (TeamType == ETeamType::ETT_Police)
	{
		return PoliceScrollBox;
	}
	if (TeamType == ETeamType::ETT_Bandit)
	{
		return BanditScrollBox;
	}
	return nullptr;
}

void UHallUserWidget::PushMessage(EChatChannel Channel, const FText& Text)
{
	//构建文本并设置到容器中
	UTextBlock* TextBlock = NewObject<UTextBlock>(ChatScrollBox);
	TextBlock->SetText(FText::Format(NSLOCTEXT("ui","cvodv1","[{0}]{1}"),LG::GetChatChannelText(Channel),Text));
	//设置颜色
	TextBlock->SetColorAndOpacity(LG::GetChatChannelColor(Channel));
	ChatScrollBox->AddChild(TextBlock);
}

void UHallUserWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (AHallGameState* Gs = Cast<AHallGameState>(GetWorld()->GetGameState()))
	{
		Gs->OnAddPlayerState.AddUObject(this, &ThisClass::OnAddPlayerState);
		Gs->OnRemovePlayerState.AddUObject(this, &ThisClass::OnRemovePlayerState);
		
		//更新面板
		HallUserInfoWidget = LoadClass<UHallUserInfoWidget>(this, TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/LegoGame/UMG/Hall/WBP_HallUserInfo.WBP_HallUserInfo_C'"));
		for (auto Ps : Gs->PlayerArray)
		{
			UHallUserInfoWidget* InfoWidget = CreateWidget<UHallUserInfoWidget>(GetOwningPlayer(),HallUserInfoWidget);
			InfoWidget->BindPlayerState(Ps);
			PoliceScrollBox->AddChild(InfoWidget);
		}
	}
	
	//判断在客户端还是服务端
	if (GetOwningPlayer()->HasAuthority())
	{
		ButtonTextBlock->SetText(NSLOCTEXT("ui","cdfakc9","开始游戏"));
		QuitButtonTextBlock->SetText(NSLOCTEXT("ui","akldsj121","解散房间"));
	}
	SubmitButton->OnClicked.AddDynamic(this,&ThisClass::OnSubmitButtonClicked);
	QuitButton->OnClicked.AddDynamic(this,&ThisClass::OnQuitButtonClicked);
}

void UHallUserWidget::OnAddPlayerState(APlayerState* PlayerState)
{
	UHallUserInfoWidget* InfoWidget = CreateWidget<UHallUserInfoWidget>(GetOwningPlayer(),HallUserInfoWidget);
	InfoWidget->BindPlayerState(PlayerState);
	PoliceScrollBox->AddChild(InfoWidget);
	
}

void UHallUserWidget::OnRemovePlayerState(APlayerState* PlayerState)
{
	for (int32 i = 0; i < PoliceScrollBox->GetChildrenCount(); ++i)
	{
		if (UHallUserInfoWidget* InfoWidget = Cast<UHallUserInfoWidget>(PoliceScrollBox->GetChildAt(i)))
		{
			if (InfoWidget->GetHallPlayerState() == PlayerState)
			{
				InfoWidget->RemoveFromParent();
				break;
			}
		}
	}
	
	for (int32 i = 0; i<BanditScrollBox->GetChildrenCount(); ++i)
	{
		if (UHallUserInfoWidget* InfoWidget = Cast<UHallUserInfoWidget>(BanditScrollBox->GetChildAt(i)))
		{
			if (InfoWidget->GetHallPlayerState() == PlayerState)
			{
				InfoWidget->RemoveFromParent();
				return;
			}
		}
	}
}


void UHallUserWidget::SendChatMessage(EChatChannel ChatChannel, const FText& Text)
{
	//PushMessage(ChatChannel, Text);
	if (AHallPlayerController* Pc = Cast<AHallPlayerController>(GetOwningPlayer()))
	{
		Pc->SendChatMessage(ChatChannel, Text);
	}
	
}

void UHallUserWidget::OnSubmitButtonClicked()
{
	if (GetOwningPlayer()->HasAuthority())
	{
		//开始游戏
		if (AHallGameMode* Gm = Cast<AHallGameMode>(GetWorld()->GetAuthGameMode()))
		{
			Gm->StartLgGame();
		}
		
	}
	else
	{
		//准备或者取消准备
		if (AHallPlayerState* Ps = GetOwningPlayer()->GetPlayerState<AHallPlayerState>())
		{
			//绑定通知只需要一次
			if (!Ps->OnReadyChanged.IsBoundToObject(this))
			{
				Ps->OnReadyChanged.AddUObject(this, &ThisClass::OnReadyChanged);
			}
			Ps->SetReady(!Ps->IsReady());
		}
	}
	
}

void UHallUserWidget::OnReadyChanged(bool bReady)
{
	if (bReady)
	{
		ButtonTextBlock->SetText(NSLOCTEXT("ui","dskfgi2","取消准备"));
	}
	else
	{
		ButtonTextBlock->SetText(NSLOCTEXT("ui","dskfgi2111","准备"));
	}
}

void UHallUserWidget::OnQuitButtonClicked()
{
	if (GetOwningPlayer()->HasAuthority())
	{
		EndGame();	
	}
	else
	{
		QuitRoom();
	}
}

void UHallUserWidget::QuitRoom()
{
	//退出游戏的房间
	GetOwningPlayer()->ClientTravel(TEXT("?close"),TRAVEL_Absolute);
	
}

void UHallUserWidget::EndGame()
{
	if (AHallGameMode* Gm = Cast<AHallGameMode>(GetWorld()->GetAuthGameMode()))
	{
		Gm->EndLgGame();
	}
	
}

