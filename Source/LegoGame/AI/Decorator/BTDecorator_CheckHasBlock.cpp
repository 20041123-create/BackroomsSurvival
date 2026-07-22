// Fill out your copyright notice in the Description page of Project Settings.


#include "BTDecorator_CheckHasBlock.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

UBTDecorator_CheckHasBlock::UBTDecorator_CheckHasBlock()
{
	NodeName = "CheckHasBlock";
	PointAKey.AddVectorFilter(this,TEXT("PointAKey"));
	PointBKey.AddVectorFilter(this,TEXT("PointBKey"));
	
	PointAKey.AddObjectFilter(this,TEXT("PointAKey"),AActor::StaticClass());
	PointBKey.AddObjectFilter(this,TEXT("PointBKey"),AActor::StaticClass());
	
	bNotifyTick = true;
	
	CheckInterval = 0.3f;
}


//返回真表示希望执行权通过，反之不希望
bool UBTDecorator_CheckHasBlock::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	//检查A点跟B点之间是否有障碍物
	if (PointAKey.IsNone()||PointBKey.IsNone())//说明没有关联黑板中的数值
	{
		return false;
	}
	
	FVector PointA;
	FVector PointB;
	
	FCollisionQueryParams Params;
	
	if (PointAKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		PointA = OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Vector>(PointAKey.GetSelectedKeyID());
	}
	else
	{
		AActor* Actor = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Object>(PointAKey.GetSelectedKeyID()));
		if (!IsValid(Actor))
		{
			return true;
		}
		PointA = Actor->GetActorLocation();
		Params.AddIgnoredActor(Actor);
	}
	
	if (PointBKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		PointB = OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Vector>(PointBKey.GetSelectedKeyID());
	}
	else
	{
		AActor* Actor = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Object>(PointBKey.GetSelectedKeyID()));
		if (!IsValid(Actor))
		{
			return true;
		}
		PointB = Actor->GetActorLocation();
		Params.AddIgnoredActor(Actor);
	}
	
	float* CheckTick = reinterpret_cast<float*>(NodeMemory);
	*CheckTick = CheckInterval;
	
	//发起检测
	FHitResult Hit;
	return OwnerComp.GetWorld()->LineTraceSingleByChannel(Hit,PointA+FVector::UpVector*OffsetA,PointB+FVector::UpVector*OffsetB,
		UEngineTypes::ConvertToCollisionChannel(TraceChannel),Params);
	
}

void UBTDecorator_CheckHasBlock::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	UBlackboardData* BBData = GetBlackboardAsset();
	if (BBData)
	{
		PointAKey.ResolveSelectedKey(*BBData);
		PointBKey.ResolveSelectedKey(*BBData);
	}
	
}

void UBTDecorator_CheckHasBlock::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	//对于装饰器来说，如果只开启Tick,那么Tick不会执行，还需要保证当前装饰器需要开启观察者终止
	float* CheckTick = reinterpret_cast<float*>(NodeMemory);
	if (*CheckTick-=DeltaSeconds<0)
	{
		//开启检查
		if (!CalculateRawConditionValue(OwnerComp,NodeMemory))//返回假说明两点之间没有障碍物了，需要立刻刷新父节点执行权
		{
			//EBTDecoratorAbortRequest此枚举是标注我们立刻终止还是等待任务结束终止
			ConditionalFlowAbort(OwnerComp,EBTDecoratorAbortRequest::ConditionPassing);
		}
	}
	
}

uint16 UBTDecorator_CheckHasBlock::GetInstanceMemorySize() const
{
	return sizeof(float);
	
}
