// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Sprint.generated.h"

/**
 * 
 */
UCLASS()
class LEGOGAME_API UBTTask_Sprint : public UBTTaskNode
{
	GENERATED_BODY()
	
	UBTTask_Sprint();
	
protected:
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
	
protected:
	
	UPROPERTY(EditAnywhere)
	bool bSprint;
	
};
