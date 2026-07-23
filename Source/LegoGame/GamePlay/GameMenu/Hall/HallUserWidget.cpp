#include "HallUserWidget.h"

#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "LegoGame/LegoGame.h"
#include "LegoGame/GamePlay/GameMenu/Hall/HallUserInfoWidget.h"
#include "LegoGame/GamePlay/Hall/HallGameState.h"
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
	UTextBlock* TextBlock = NewObject<UTextBlock>(ChatScrollBox);
	TextBlock->SetText(FText::Format(
		NSLOCTEXT("ui", "cvodv1", "[{0}]{1}"),
		LG::GetChatChannelText(Channel),
		Text));
	TextBlock->SetColorAndOpacity(LG::GetChatChannelColor(Channel));
	ChatScrollBox->AddChild(TextBlock);
}

void UHallUserWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (AHallGameState* GameState = GetWorld()->GetGameState<AHallGameState>())
	{
		GameState->OnAddPlayerState.AddUObject(this, &ThisClass::OnAddPlayerState);
		GameState->OnRemovePlayerState.AddUObject(this, &ThisClass::OnRemovePlayerState);

		HallUserInfoWidget = LoadClass<UHallUserInfoWidget>(
			this,
			TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/LegoGame/UMG/Hall/WBP_HallUserInfo.WBP_HallUserInfo_C'"));
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			OnAddPlayerState(PlayerState);
		}
	}

	if (AHallPlayerState* OwningPlayerState = GetOwningPlayer()->GetPlayerState<AHallPlayerState>())
	{
		OwningPlayerState->OnIsMaster.AddUObject(this, &ThisClass::OnBecameMaster);
		if (OwningPlayerState->IsMaster())
		{
			OnBecameMaster();
		}
	}

	SubmitButton->OnClicked.AddDynamic(this, &ThisClass::OnSubmitButtonClicked);
	QuitButton->OnClicked.AddDynamic(this, &ThisClass::OnQuitButtonClicked);
}

void UHallUserWidget::OnAddPlayerState(APlayerState* PlayerState)
{
	if (!HallUserInfoWidget || !PlayerState)
	{
		return;
	}

	UHallUserInfoWidget* InfoWidget = CreateWidget<UHallUserInfoWidget>(
		GetOwningPlayer(), HallUserInfoWidget);
	if (InfoWidget)
	{
		InfoWidget->BindPlayerState(PlayerState);
		if (!InfoWidget->GetParent())
		{
			PoliceScrollBox->AddChild(InfoWidget);
		}
	}
}

void UHallUserWidget::OnRemovePlayerState(APlayerState* PlayerState)
{
	const auto RemoveFromScrollBox = [PlayerState](UScrollBox* ScrollBox)
	{
		if (!ScrollBox)
		{
			return false;
		}
		for (int32 Index = 0; Index < ScrollBox->GetChildrenCount(); ++Index)
		{
			if (UHallUserInfoWidget* InfoWidget =
				Cast<UHallUserInfoWidget>(ScrollBox->GetChildAt(Index)))
			{
				if (InfoWidget->GetHallPlayerState() == PlayerState)
				{
					InfoWidget->RemoveFromParent();
					return true;
				}
			}
		}
		return false;
	};

	if (!RemoveFromScrollBox(PoliceScrollBox))
	{
		RemoveFromScrollBox(BanditScrollBox);
	}
}

void UHallUserWidget::SendChatMessage(EChatChannel ChatChannel, const FText& Text)
{
	if (AHallPlayerController* PlayerController = Cast<AHallPlayerController>(GetOwningPlayer()))
	{
		PlayerController->SendChatMessage(ChatChannel, Text);
	}
}

void UHallUserWidget::OnSubmitButtonClicked()
{
	AHallPlayerState* OwningPlayerState =
		GetOwningPlayer()->GetPlayerState<AHallPlayerState>();
	if (!OwningPlayerState)
	{
		return;
	}

	if (OwningPlayerState->IsMaster())
	{
		if (AHallPlayerController* PlayerController =
			Cast<AHallPlayerController>(GetOwningPlayer()))
		{
			PlayerController->RequestStartGame();
		}
		return;
	}

	if (!OwningPlayerState->OnReadyChanged.IsBoundToObject(this))
	{
		OwningPlayerState->OnReadyChanged.AddUObject(this, &ThisClass::OnReadyChanged);
	}
	OwningPlayerState->SetReady(!OwningPlayerState->IsReady());
}

void UHallUserWidget::OnReadyChanged(bool bReady)
{
	ButtonTextBlock->SetText(
		bReady
			? NSLOCTEXT("ui", "CancelReady", "取消准备")
			: NSLOCTEXT("ui", "Ready", "准备"));
}

void UHallUserWidget::OnBecameMaster()
{
	ButtonTextBlock->SetText(NSLOCTEXT("ui", "MasterStartGame", "开始游戏"));
	QuitButtonTextBlock->SetText(NSLOCTEXT("ui", "MasterDismissRoom", "解散房间"));
}

void UHallUserWidget::OnQuitButtonClicked()
{
	const AHallPlayerState* OwningPlayerState =
		GetOwningPlayer()->GetPlayerState<AHallPlayerState>();
	if (OwningPlayerState && OwningPlayerState->IsMaster())
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
	GetOwningPlayer()->ClientTravel(TEXT("?close"), TRAVEL_Absolute);
}

void UHallUserWidget::EndGame()
{
	if (AHallPlayerController* PlayerController = Cast<AHallPlayerController>(GetOwningPlayer()))
	{
		PlayerController->RequestEndGame();
	}
}
