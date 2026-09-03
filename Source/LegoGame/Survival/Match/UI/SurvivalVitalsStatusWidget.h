#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LegoGame/Survival/Contracts/SurvivalTypes.h"
#include "SurvivalVitalsStatusWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;
struct FSurvivalVitalsStatusWidgetTestAccess;

/**
 * Production player-needs overlay. WBP_PlayerState supplies its two roulette
 * images while this class reads only public Survival runtime interfaces.
 */
UCLASS()
class LEGOGAME_API USurvivalVitalsStatusWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> PowerProgress;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> SANProgress;

private:
	void RefreshVitals();
	static float NormalizeVital(float CurrentValue, float MaxValue);
	static ESurvivalMatchPhase ResolveDisplayPhase(ESurvivalMatchPhase SurvivalMatchPhase, bool bEngineMatchStarted);
	static bool ShouldDisplayVitals(ESurvivalMatchPhase MatchPhase, bool bHasVitalsPawn,
		const FSurvivalVitalsSnapshot& Snapshot);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PowerProgressMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SANProgressMaterial;

	float NextRefreshWorldTime = 0.0f;

	friend struct FSurvivalVitalsStatusWidgetTestAccess;
};
