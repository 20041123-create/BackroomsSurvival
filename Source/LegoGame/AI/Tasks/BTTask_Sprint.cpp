// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_Sprint.h"

#include "AIController.h"
#include "LegoGame/Character/LgCharacterBase.h"

UBTTask_Sprint::UBTTask_Sprint()
{
	NodeName = TEXT("SprintON/Off");
	bSprint = true;
}

EBTNodeResult::Type UBTTask_Sprint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (ALgCharacterBase* Character = Cast<ALgCharacterBase>(OwnerComp.GetAIOwner()->GetPawn()))
	{
		if (bSprint)
		{
			Character->StartSprint();
		}
		else
		{
			Character->StopSprint();
		}
		return EBTNodeResult::Succeeded;
	}
	
	return EBTNodeResult::Failed; 
}
