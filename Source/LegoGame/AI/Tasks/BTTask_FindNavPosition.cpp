// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_FindNavPosition.h"

#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

UBTTask_FindNavPosition::UBTTask_FindNavPosition()
{
	//重命名
	NodeName = TEXT("FindNavPosition");
	//设置键值约束器
	NavPositionKey.AddVectorFilter(this,TEXT("NavPositionKey"));
	
	OriginPositionKey.AddVectorFilter(this,TEXT("OriginPositionKey"));
	OriginPositionKey.AddObjectFilter(this,TEXT("OriginPositionKey"),AActor::StaticClass());
	
	Radius = 500.0f;
}

EBTNodeResult::Type UBTTask_FindNavPosition::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	//检查黑板键值拾取器是否有效
	if (NavPositionKey.IsNone()||OriginPositionKey.IsNone())
	{
		return EBTNodeResult::Failed;
	}
	
	// //基于当前行为树角色位置，随机一个角度，找一个随即长度的位置
	// FVector Location = OwnerComp.GetAIOwner()->GetPawn()->GetActorLocation();
	// //随机一个方向
	// FVector Dir = FVector::ForwardVector.RotateAngleAxis(FMath::FRandRange(-180.f, 180.f), FVector::UpVector);
	// //点
	// FVector Pointer = Location + Dir*FMath::FRandRange(500.f, 1000.f);
	// //向黑板中装填数据
	// //OwnerComp.GetBlackboardComponent()->SetValueAsVector(TEXT("NavPosition"), Pointer);
	// OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(NavPositionKey.GetSelectedKeyID(), Pointer);
	
	//判断当前键值拾取器选择的数据类型是什么
	FVector OriginPosition;
	if (OriginPositionKey.SelectedKeyType==UBlackboardKeyType_Vector::StaticClass())
	{
		OriginPosition = OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Vector>(OriginPositionKey.GetSelectedKeyID());
	}
	else
	{
		const AActor* OriginActor = Cast<AActor>( OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Object>(OriginPositionKey.GetSelectedKeyID()));
		if (!IsValid(OriginActor))
		{
			return EBTNodeResult::Failed;
		}
		OriginPosition = OriginActor->GetActorLocation();
	}
	FVector NavPosition;
	//拾取位置,导航中的
	if (UNavigationSystemV1::K2_GetRandomLocationInNavigableRadius(OwnerComp.GetAIOwner(),OriginPosition,NavPosition,Radius))
	{
		
		OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(NavPositionKey.GetSelectedKeyID(),NavPosition);
		return EBTNodeResult::Succeeded;
	}
	
	return EBTNodeResult::Failed;
	
}

void UBTTask_FindNavPosition::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	
	UBlackboardData* BBData = GetBlackboardAsset();
	if (BBData)
	{
		//将键值拾取器关联到黑板数据
		NavPositionKey.ResolveSelectedKey(*BBData);
		OriginPositionKey.ResolveSelectedKey(*BBData);
	}
}
