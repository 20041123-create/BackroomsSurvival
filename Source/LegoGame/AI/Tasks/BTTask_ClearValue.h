// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ClearValue.generated.h"

/**
 * 
 */
UCLASS()
class LEGOGAME_API UBTTask_ClearValue : public UBTTaskNode
{
	GENERATED_BODY()
	
	UBTTask_ClearValue();
	
protected:
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	
	
protected:
	UPROPERTY(EditAnywhere)
	FBlackboardKeySelector KeySelector;
	
};
