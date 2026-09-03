#pragma once

#include "CoreMinimal.h"
#include "LegoGame/GamePlay/MainGame/LgHUD.h"
#include "SurvivalHUD.generated.h"

class USurvivalHUDWidget;
class USurvivalEndingWidget;
class USurvivalRespawnWidget;
class USurvivalVitalsStatusWidget;

/** Adds the replicated Survival overlay while retaining the existing package HUD behavior. */
UCLASS()
class LEGOGAME_API ASurvivalHUD : public ALgHUD
{
	GENERATED_BODY()

public:
	ASurvivalHUD();

protected:
	virtual void BeginPlay() override;

private:
	/** Resolved during UObject construction; runtime BeginPlay never invokes ConstructorHelpers. */
	UPROPERTY(EditDefaultsOnly, Category="Survival|UI")
	TSubclassOf<USurvivalHUDWidget> SurvivalHUDWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<USurvivalHUDWidget> SurvivalHUDWidget;

	/** WBP_PlayerState: material-driven hunger and thirst status rings. */
	UPROPERTY(EditDefaultsOnly, Category="Survival|UI")
	TSubclassOf<USurvivalVitalsStatusWidget> PlayerVitalsWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<USurvivalVitalsStatusWidget> PlayerVitalsWidget;

	/** WBP_SurvivalRespawn: replicated respawn queue and spectator presentation. */
	UPROPERTY(EditDefaultsOnly, Category="Survival|UI")
	TSubclassOf<USurvivalRespawnWidget> RespawnWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<USurvivalRespawnWidget> RespawnWidget;

	/** WBP_Ending: replicated match outcome and return-to-hall action. */
	UPROPERTY(EditDefaultsOnly, Category="Survival|UI")
	TSubclassOf<USurvivalEndingWidget> EndingWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<USurvivalEndingWidget> EndingWidget;
};
