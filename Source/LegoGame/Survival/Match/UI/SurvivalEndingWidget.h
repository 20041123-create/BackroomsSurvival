#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LegoGame/Survival/Contracts/SurvivalTypes.h"
#include "LegoGame/Survival/Match/SurvivalMatchTypes.h"
#include "SurvivalEndingWidget.generated.h"

class ASurvivalGameState;
class UButton;
class UTextBlock;
struct FSurvivalEndingWidgetTestAccess;

/** Client-only presentation adapter for WBP_Ending. Match outcome remains server authoritative. */
UCLASS()
class LEGOGAME_API USurvivalEndingWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock_37;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> Button_61;

private:
	void BindGameState();
	void UnbindGameState();
	void HandleMatchStateChanged();
	void RefreshPresentation();
	void SetEndingVisible(bool bVisible);

	UFUNCTION()
	void HandleReturnToHallClicked();

	static FText ResolveResultText(ESurvivalMatchOutcome Outcome, ETeamType LocalTeam);

	TWeakObjectPtr<ASurvivalGameState> BoundGameState;
	float NextRefreshWorldTime = 0.0f;
	bool bEndingVisible = false;
	bool bTravelRequested = false;

	friend struct FSurvivalEndingWidgetTestAccess;
};
