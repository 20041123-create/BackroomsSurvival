// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_WaitTime.h"

UBTTask_WaitTime::UBTTask_WaitTime()
{
	NodeName = TEXT("WaitTime");
	WaitTime = 5.f;
	RandomDeviation = 0.f;
	bNotifyTick = true;
	//bCreateNodeInstance = true;//为每一个使用行为树的对象对于当前节点创建一个全新的节点
}

EBTNodeResult::Type UBTTask_WaitTime::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	
	if (WaitTime <= 0.f)
	{
		return EBTNodeResult::Succeeded;
	}
	float* pRemainingTime = reinterpret_cast<float*>(NodeMemory);
	if (RandomDeviation <= 0.f)
	{
		*pRemainingTime = WaitTime;
	}
	else
	{
		*pRemainingTime = FMath::RandRange(WaitTime-RandomDeviation, WaitTime+RandomDeviation);
	}
	return EBTNodeResult::InProgress;
	
}

void UBTTask_WaitTime::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	float* pRemainingTime = reinterpret_cast<float*>(NodeMemory);
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);
	
	//倒计时减少时间
	if ((*pRemainingTime -= DeltaSeconds) < 0.f)
	{
		//终止等待
		FinishLatentTask(OwnerComp,EBTNodeResult::Succeeded);//结束等待遗留任务
	}
}

uint16 UBTTask_WaitTime::GetInstanceMemorySize() const
{
	return sizeof(float);
	
}
