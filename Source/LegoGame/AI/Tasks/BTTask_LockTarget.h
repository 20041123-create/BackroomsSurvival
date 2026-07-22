// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_LockTarget.generated.h"

/**
 * 
 */
UCLASS()
class LEGOGAME_API UBTTask_LockTarget : public UBTTaskNode
{
	GENERATED_BODY()
	
	UBTTask_LockTarget();
	
	
protected:
	
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
protected:
	
	UPROPERTY(EditAnywhere)
	bool bLockTarget;
	
	UPROPERTY(EditAnywhere)
	FBlackboardKeySelector TargetKey;
	
};
