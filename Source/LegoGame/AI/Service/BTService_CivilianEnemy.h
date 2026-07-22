// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_CivilianEnemy.generated.h"

/**
 * 
 */
UCLASS()
class LEGOGAME_API UBTService_CivilianEnemy : public UBTService
{
	GENERATED_BODY()
	
	UBTService_CivilianEnemy();
	
protected:
	
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
