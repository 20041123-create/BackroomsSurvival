#include "SurvivalHUD.h"

#include "LegoGame/Survival/Match/UI/SurvivalEndingWidget.h"
#include "LegoGame/Survival/Match/UI/SurvivalHUDWidget.h"
#include "LegoGame/Survival/Match/UI/SurvivalRespawnWidget.h"
#include "LegoGame/Survival/Match/UI/SurvivalVitalsStatusWidget.h"
#include "UObject/ConstructorHelpers.h"

ASurvivalHUD::ASurvivalHUD()
{
	SurvivalHUDWidgetClass = USurvivalHUDWidget::StaticClass();
	static ConstructorHelpers::FClassFinder<USurvivalHUDWidget> SurvivalHUDWidgetClassFinder(
		TEXT("/Game/LegoGame/Survival/UI/WBP_SurvivalHUD"));
	if (SurvivalHUDWidgetClassFinder.Succeeded())
	{
		SurvivalHUDWidgetClass = SurvivalHUDWidgetClassFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<USurvivalVitalsStatusWidget> PlayerVitalsWidgetClassFinder(
		TEXT("/Game/LegoGame/UMG/Game/WBP_PlayerState"));
	if (PlayerVitalsWidgetClassFinder.Succeeded())
	{
		PlayerVitalsWidgetClass = PlayerVitalsWidgetClassFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<USurvivalRespawnWidget> RespawnWidgetClassFinder(
		TEXT("/Game/LegoGame/Survival/UI/WBP_SurvivalRespawn"));
	if (RespawnWidgetClassFinder.Succeeded())
	{
		RespawnWidgetClass = RespawnWidgetClassFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<USurvivalEndingWidget> EndingWidgetClassFinder(
		TEXT("/Game/LegoGame/UMG/Game/WBP_Ending"));
	if (EndingWidgetClassFinder.Succeeded())
	{
		EndingWidgetClass = EndingWidgetClassFinder.Class;
	}
}

void ASurvivalHUD::BeginPlay()
{
	Super::BeginPlay();

	if (GetNetMode() == NM_DedicatedServer || !GetOwningPlayerController())
	{
		return;
	}

	TSubclassOf<USurvivalHUDWidget> WidgetClass = SurvivalHUDWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = USurvivalHUDWidget::StaticClass();
	}
	SurvivalHUDWidget = CreateWidget<USurvivalHUDWidget>(GetOwningPlayerController(), WidgetClass);
	if (SurvivalHUDWidget)
	{
		SurvivalHUDWidget->AddToViewport(10);
	}

	if (!PlayerVitalsWidgetClass)
	{
		// Supports an editor session that was already open when WBP_PlayerState
		// was reparented to the native status widget.
		PlayerVitalsWidgetClass = LoadClass<USurvivalVitalsStatusWidget>(nullptr,
			TEXT("/Game/LegoGame/UMG/Game/WBP_PlayerState.WBP_PlayerState_C"));
	}
	if (!PlayerVitalsWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Survival player-vitals HUD was not created: WBP_PlayerState class is unavailable."));
	}
	else
	{
		PlayerVitalsWidget = CreateWidget<USurvivalVitalsStatusWidget>(GetOwningPlayerController(), PlayerVitalsWidgetClass);
		if (!PlayerVitalsWidget)
		{
			UE_LOG(LogTemp, Warning, TEXT("Survival player-vitals HUD was not created: CreateWidget failed."));
		}
		else
		{
			PlayerVitalsWidget->AddToViewport(20);
			UE_LOG(LogTemp, Display, TEXT("Survival player-vitals HUD created from %s."),
				*PlayerVitalsWidgetClass->GetPathName());
		}
	}

	if (!RespawnWidgetClass)
	{
		RespawnWidgetClass = LoadClass<USurvivalRespawnWidget>(nullptr,
			TEXT("/Game/LegoGame/Survival/UI/WBP_SurvivalRespawn.WBP_SurvivalRespawn_C"));
	}
	if (!RespawnWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Survival respawn HUD was not created: WBP_SurvivalRespawn class is unavailable."));
		return;
	}

	RespawnWidget = CreateWidget<USurvivalRespawnWidget>(GetOwningPlayerController(), RespawnWidgetClass);
	if (!RespawnWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("Survival respawn HUD was not created: CreateWidget failed."));
		return;
	}

	RespawnWidget->AddToViewport(30);
	UE_LOG(LogTemp, Display, TEXT("Survival respawn HUD created from %s."), *RespawnWidgetClass->GetPathName());

	if (!EndingWidgetClass)
	{
		EndingWidgetClass = LoadClass<USurvivalEndingWidget>(nullptr,
			TEXT("/Game/LegoGame/UMG/Game/WBP_Ending.WBP_Ending_C"));
	}
	if (!EndingWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Survival ending HUD was not created: WBP_Ending class is unavailable."));
		return;
	}

	EndingWidget = CreateWidget<USurvivalEndingWidget>(GetOwningPlayerController(), EndingWidgetClass);
	if (!EndingWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("Survival ending HUD was not created: CreateWidget failed."));
		return;
	}

	EndingWidget->AddToViewport(40);
	UE_LOG(LogTemp, Display, TEXT("Survival ending HUD created from %s."), *EndingWidgetClass->GetPathName());
}
