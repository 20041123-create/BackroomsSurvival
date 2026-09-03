#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFetureUserWidget.generated.h"

class UTextBlock;

/** Main gameplay overlay implemented by WBP_GameFeture. */
UCLASS()
class LEGOGAME_API UGameFetureUserWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void RefreshWeaponAmmo();

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> WeaponClipTextBlock;

private:
	float NextAmmoRefreshWorldTime = 0.0f;
};
