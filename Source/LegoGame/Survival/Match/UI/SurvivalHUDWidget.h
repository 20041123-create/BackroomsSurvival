#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SurvivalHUDWidget.generated.h"

class ASurvivalGameState;
class ASurvivalPlayerState;
class UTextBlock;

/**
 * Native, asset-free Survival HUD fallback. Widget blueprints in Content may
 * subclass this class later without changing any replication bindings.
 */
UCLASS()
class LEGOGAME_API USurvivalHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildFallbackLayout();
	void BindSurvivalState();
	void UnbindSurvivalState();
	void HandleSurvivalStateChanged();
	void HandlePlayerLifeStateChanged();
	void EnsureVitalsDisplay();
	void Refresh();
	FText GetVitalsText() const;
	FText GetPhaseText() const;
	FText GetOutcomeText() const;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PhaseText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TeamText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> VitalsText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DirectorText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RespawnText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> OutcomeText;

	TWeakObjectPtr<ASurvivalGameState> BoundGameState;
	TWeakObjectPtr<ASurvivalPlayerState> BoundPlayerState;
	float NextRefreshWorldTime = 0.0f;
};
