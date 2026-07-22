// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_LockTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

UBTTask_LockTarget::UBTTask_LockTarget()
{
	NodeName = TEXT("LockTarget");
	bLockTarget = true;
	
	TargetKey.AddObjectFilter(this,TEXT("TargetKey"),AActor::StaticClass());
	
	
	
	
}

void UBTTask_LockTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	UBlackboardData* BBData = GetBlackboardAsset();
	if (BBData)
	{
		TargetKey.ResolveSelectedKey(*BBData);
	}
	
}

EBTNodeResult::Type UBTTask_LockTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (TargetKey.IsNone())
	{
		return EBTNodeResult::Failed;
	}
	AActor* Actor = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Object>(TargetKey.GetSelectedKeyID()));
	if (IsValid(Actor))
	{
		if (bLockTarget)
		{
			OwnerComp.GetAIOwner()->SetFocus(Actor);//聚焦到目标actor,controller会一直朝向目标
		}
		else
		{
			OwnerComp.GetAIOwner()->ClearFocus(EAIFocusPriority::Gameplay);
		}
		return EBTNodeResult::Succeeded;
	}
	return EBTNodeResult::Failed;	
	
}
