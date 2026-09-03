#include "GameFetureUserWidget.h"

#include "Components/TextBlock.h"
#include "LegoGame/Survival/Contracts/SurvivalInterfaces.h"

void UGameFetureUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshWeaponAmmo();
}

void UGameFetureUserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (GetWorld() && GetWorld()->GetTimeSeconds() >= NextAmmoRefreshWorldTime)
	{
		NextAmmoRefreshWorldTime = GetWorld()->GetTimeSeconds() + 0.1f;
		RefreshWeaponAmmo();
	}
}

void UGameFetureUserWidget::RefreshWeaponAmmo()
{
	if (!WeaponClipTextBlock)
	{
		return;
	}

	APawn* Pawn = GetOwningPlayerPawn();
	if (!Pawn || !Pawn->GetClass()->ImplementsInterface(USurvivalWeaponStateInterface::StaticClass()))
	{
		WeaponClipTextBlock->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	const FSurvivalWeaponAmmoSnapshot Snapshot =
		ISurvivalWeaponStateInterface::Execute_GetSurvivalWeaponAmmoSnapshot(Pawn);
	if (!Snapshot.bHasEquippedWeapon)
	{
		WeaponClipTextBlock->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	WeaponClipTextBlock->SetVisibility(ESlateVisibility::Visible);
	WeaponClipTextBlock->SetText(FText::Format(
		NSLOCTEXT("Survival", "GameFeatureAmmoStatus", "Ammo {0}/{1}  Reserve {2}"),
		Snapshot.LoadedAmmo,
		Snapshot.ClipCapacity,
		Snapshot.ReserveAmmo));
}
