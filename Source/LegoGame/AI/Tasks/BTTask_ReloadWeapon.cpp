// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_ReloadWeapon.h"

#include "AIController.h"
#include "LegoGame/Character/LgCharacterBase.h"
#include "LegoGame/Weapon/WeaponBase.h"

UBTTask_ReloadWeapon::UBTTask_ReloadWeapon()
{
	NodeName = TEXT("ReLoadWeapon");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_ReloadWeapon::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (ALgCharacterBase* Character = Cast<ALgCharacterBase>(OwnerComp.GetAIOwner()->GetPawn()))
	{
		if (Character->GetHoldWeapon())
		{
			Character->GetHoldWeapon()->ReloadClip();
			return EBTNodeResult::InProgress;
		}
	}
	return EBTNodeResult::Failed;
	
}

void UBTTask_ReloadWeapon::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);
	
	//检查武器是否换弹完成，如果完成则继续执行任务
	if (ALgCharacterBase* Character = Cast<ALgCharacterBase>(OwnerComp.GetAIOwner()->GetPawn()))
	{
		if (Character->GetHoldWeapon()&&Character->GetHoldWeapon()->GetCurrentState()==EWeaponState::EWS_Normal)
		{
			FinishLatentTask(OwnerComp,EBTNodeResult::Succeeded);
		}
	}
}
