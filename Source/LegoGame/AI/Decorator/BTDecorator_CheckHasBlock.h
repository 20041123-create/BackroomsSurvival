// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_ReachedMoveGoal.h"
#include "BTDecorator_CheckHasBlock.generated.h"

/**
 * 
 */
UCLASS()
class LEGOGAME_API UBTDecorator_CheckHasBlock : public UBTDecorator
{
	GENERATED_BODY()
	
	UBTDecorator_CheckHasBlock();
	
protected:
	
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	
	virtual uint16 GetInstanceMemorySize() const override;
	
	
protected:
	
	UPROPERTY(EditAnywhere)
	FBlackboardKeySelector PointAKey;
	UPROPERTY(EditAnywhere)
	FBlackboardKeySelector PointBKey;
	
	UPROPERTY(EditAnywhere)
	TEnumAsByte<ETraceTypeQuery> TraceChannel;
	
	UPROPERTY(EditAnywhere)
	float OffsetA;
	UPROPERTY(EditAnywhere)
	float OffsetB;
	
	
	UPROPERTY(EditAnywhere)
	float CheckInterval;
};
