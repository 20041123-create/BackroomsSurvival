// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindNavPosition.generated.h"

/**
 * 
 */
UCLASS()
class LEGOGAME_API UBTTask_FindNavPosition : public UBTTaskNode
{
	GENERATED_BODY()
	UBTTask_FindNavPosition();
	
protected:
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	
	
	
protected:
	
	UPROPERTY(EditAnywhere)
	FBlackboardKeySelector NavPositionKey;
	UPROPERTY(EditAnywhere)
	FBlackboardKeySelector OriginPositionKey;
	UPROPERTY(EditAnywhere)
	float Radius;
};
