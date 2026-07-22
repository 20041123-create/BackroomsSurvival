// Fill out your copyright notice in the Description page of Project Settings.


#include "Notify_Reload.h"

#include "LegoGame/Character/LgCharacterBase.h"
#include "LegoGame/Weapon/WeaponBase.h"

void UNotify_Reload::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                            const FAnimNotifyEventReference& EventReference)
{
	//回填子弹
	if (ALgCharacterBase* CharacterBase = Cast<ALgCharacterBase>(MeshComp->GetOwner()))
	{
		if (CharacterBase->GetHoldWeapon())
		{
			CharacterBase->GetHoldWeapon()->MakeFullClip();
		}
	}
	
}
