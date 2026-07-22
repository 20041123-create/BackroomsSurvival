// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFetureUserWidget.h"

#include "Components/TextBlock.h"
#include "LegoGame/Components/PackageComponent.h"
#include "LegoGame/Player/PlayerCharacter.h"
#include "LegoGame/Weapon/WeaponBase.h"

void UGameFetureUserWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwningPlayerPawn()))
	{
		if (Player->GetPackageComponent())
		{
			Player->GetPackageComponent()->OnEquipWeapon.AddUObject(this,&ThisClass::OnEquipWeapon);
			Player->GetPackageComponent()->OnUnEquipWeapon.AddUObject(this,&ThisClass::OnUnEquipWeapon);
		}
	}
	
}

void UGameFetureUserWidget::OnEquipWeapon(int32 ID)
{
	WeaponClipTextBlock->SetVisibility(ESlateVisibility::Visible);
	//更新子弹
	if (APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwningPlayerPawn()))
	{
		if (Player->GetHoldWeapon())
		{
			Player->GetHoldWeapon()->OnWeaponClipChanged.AddUObject(this,&ThisClass::OnWeaponClipChanged);
			OnWeaponClipChanged(Player->GetHoldWeapon()->GetCurrentClipVolume(),Player->GetHoldWeapon()->GetMaxClipVolume());
		}
	}
}

void UGameFetureUserWidget::OnUnEquipWeapon(int32 ID)
{
	WeaponClipTextBlock->SetVisibility(ESlateVisibility::Hidden);
	
}

void UGameFetureUserWidget::OnWeaponClipChanged(int32 CurrClipVolume, int32 MaxClipVolume)
{
	//更新UI
	WeaponClipTextBlock->SetText(FText::Format(NSLOCTEXT("ui","classgt3","{0}/{1}"),FText::AsNumber(CurrClipVolume),FText::AsNumber(MaxClipVolume)));
}
