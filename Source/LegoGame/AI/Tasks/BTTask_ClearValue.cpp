// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_ClearValue.h"

#include "BehaviorTree/BlackboardComponent.h"

UBTTask_ClearValue::UBTTask_ClearValue()
{
	NodeName = TEXT("ClearBlackBoardValue");
	
	
	
}

EBTNodeResult::Type UBTTask_ClearValue::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (KeySelector.IsNone())
	{
		return EBTNodeResult::Failed;
	}
	//清理黑板数据
	OwnerComp.GetBlackboardComponent()->ClearValue(KeySelector.GetSelectedKeyID());
	
	return EBTNodeResult::Succeeded;
}

void UBTTask_ClearValue::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	UBlackboardData* BBData = GetBlackboardAsset();
	if (BBData)
	{
		KeySelector.ResolveSelectedKey(*BBData);
	}
	
}
