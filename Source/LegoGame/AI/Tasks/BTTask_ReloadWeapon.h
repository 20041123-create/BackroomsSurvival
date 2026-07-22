// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ReloadWeapon.generated.h"

/**
 * 
 */
UCLASS()
class LEGOGAME_API UBTTask_ReloadWeapon : public UBTTaskNode
{
	GENERATED_BODY()
	
	UBTTask_ReloadWeapon();
	
protected:
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
