// Fill out your copyright notice in the Description page of Project Settings.


#include "HallUserInfoWidget.h"

#include "HallUserWidget.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "LegoGame/LegoGame.h"
#include "LegoGame/GamePlay/Hall/HallGameMode.h"
#include "LegoGame/GamePlay/Hall/HallHUD.h"
#include "LegoGame/GamePlay/Hall/HallPlayerState.h"

void UHallUserInfoWidget::BindPlayerState(APlayerState* PlayerState)
{
	if (AHallPlayerState* Ps = Cast<AHallPlayerState>(PlayerState))
	{
		Ps->OnPlayerInfoChanged.AddUObject(this, &ThisClass::OnPlayerInfoChanged);
		Ps->OnReadyChanged.AddUObject(this, &ThisClass::OnPlayerReadyChanged);
		Ps->OnIsMaster.AddUObject(this, &ThisClass::OnMaster);
		//状态追赶
		if (Ps->IsMaster())
		{
			OnMaster();
		}
		else if (Ps->HasAuthority())
		{
			KickButton->SetVisibility(ESlateVisibility::Visible);
			KickButton->OnClicked.AddDynamic(this, &ThisClass::OnKickPlayer);
		}
		BindHallPlayerState = Ps;
	}
	
}

void UHallUserInfoWidget::OnPlayerInfoChanged(int32 HeadIndex, const FString& PlayerName, ETeamType TeamType,
	EJobType JobType)
{
	OnHeadIndexChanged(HeadIndex);
	PlayerNameTextBlock->SetText(FText::FromString(PlayerName));
	JobTextBlock->SetText(LG::GetJobText(JobType));
	//调整阵营关系
	if (AHallHUD* HUD = Cast<AHallHUD>(GetOwningPlayer()->GetHUD()))
	{
		UScrollBox* ScrollBox = HUD->GetHallUserWidget()->GetScrollBox(TeamType);
		if (ScrollBox && ScrollBox!=GetParent())
		{
			ScrollBox->AddChild(this);
		}
	}
}

void UHallUserInfoWidget::OnPlayerReadyChanged(bool bReady)
{
	FLinearColor Color(0,0.3f,0.3f,1.0f);
	if (bReady)
	{
		Color = FLinearColor(0.7f,0.4f,0.1f,1.0f);
	}
	BgBorder->SetBrushColor(Color);
	
}

void UHallUserInfoWidget::OnMaster()
{
	BgBorder->SetBrushColor(FLinearColor(0.3f,1.0f,0.1f,1.0f));
	
}

void UHallUserInfoWidget::OnKickPlayer()
{
	//移除玩家
	if (AHallGameMode* Gm = Cast<AHallGameMode>(GetWorld()->GetAuthGameMode()))
	{
		Gm->KickPlayer(Cast<APlayerController>(BindHallPlayerState->GetOwningController()));
	}
	
}
