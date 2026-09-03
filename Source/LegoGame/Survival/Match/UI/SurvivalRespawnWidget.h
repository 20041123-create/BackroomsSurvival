#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LegoGame/Survival/Contracts/SurvivalTypes.h"
#include "SurvivalRespawnWidget.generated.h"

class ASurvivalPlayerState;
class UTextBlock;
struct FSurvivalRespawnWidgetTestAccess;

struct FSurvivalRespawnPresentation
{
	bool bVisible = false;
	int32 CountdownSeconds = INDEX_NONE;
	FText StatusText;
};

/** Client presentation for WBP_SurvivalRespawn; gameplay remains server authoritative. */
UCLASS()
class LEGOGAME_API USurvivalRespawnWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock_Num;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock_Show;

private:
	void BindPlayerState();
	void UnbindPlayerState();
	void HandlePlayerStateChanged();
	void RefreshPresentation();
	static FSurvivalRespawnPresentation ResolvePresentation(
		ESurvivalLifeState LifeState,
		bool bMatchParticipant,
		int32 QueuePosition,
		float RespawnReadyServerTime,
		float ServerTimeSeconds,
		ESurvivalMatchPhase MatchPhase);

	TWeakObjectPtr<ASurvivalPlayerState> BoundPlayerState;
	float NextRefreshWorldTime = 0.0f;

	friend struct FSurvivalRespawnWidgetTestAccess;
};
