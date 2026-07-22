// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_Fire.h"

#include "AIController.h"
#include "LegoGame/Character/LgCharacterBase.h"

UBTTask_Fire::UBTTask_Fire()
{
	NodeName = TEXT("FireStart/End");
	bFire = true;
	
}

EBTNodeResult::Type UBTTask_Fire::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (ALgCharacterBase* Character = Cast<ALgCharacterBase>(OwnerComp.GetAIOwner()->GetPawn()))
	{
		if (bFire)
		{
			Character->StartIronSight();
			Character->StartFire();
		}
		else
		{
			Character->StopFire();
			Character->StopIronSight();
		}
		return EBTNodeResult::Succeeded;
	}
	return EBTNodeResult::Failed;
	
}
