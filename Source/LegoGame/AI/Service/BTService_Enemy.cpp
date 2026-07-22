// Fill out your copyright notice in the Description page of Project Settings.


#include "BTService_Enemy.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "LegoGame/Character/LgCharacterBase.h"
#include "LegoGame/Components/PackageComponent.h"

UBTService_Enemy::UBTService_Enemy()
{
	NodeName = TEXT("UpdateService");
	//由于此service需要接受通知，所以不能多人共用一个service，我们需要创建独立的实例
	bCreateNodeInstance = true;
	
	TargetKey.AddObjectFilter(this,TEXT("TargetKey"),AActor::StaticClass());
}

void UBTService_Enemy::OnInstanceCreated(UBehaviorTreeComponent& OwnerComp)
{
	Super::OnInstanceCreated(OwnerComp);
	BehaviorTreeComponent = &OwnerComp;
	//绑定代理
	if (ALgCharacterBase* MySelfCharacter = Cast<ALgCharacterBase>(OwnerComp.GetAIOwner()->GetPawn()))
	{
		MySelfCharacter->OnReceiveDamage.AddUObject(this,&ThisClass::OnReceiveDamage);
		MySelfCharacter->GetPackageComponent()->OnEquipWeapon.AddUObject(this,&ThisClass::OnEquipWeapon);
	}
	
}

void UBTService_Enemy::OnReceiveDamage(ALgCharacterBase* Target)
{
	if (BehaviorTreeComponent)
	{
		//比较谁离我最近作为第一攻击目标
		const AActor* OldTarget = Cast<AActor>(BehaviorTreeComponent->GetBlackboardComponent()->GetValue<UBlackboardKeyType_Object>(TargetKey.GetSelectedKeyID()));
		if (IsValid(OldTarget))
		{
			//比较距离
			const FVector StandLocation = BehaviorTreeComponent->GetAIOwner()->GetPawn()->GetActorLocation();
			if ((OldTarget->GetActorLocation()-StandLocation).Length()<(Target->GetActorLocation()-StandLocation).Length())
			{
				return;
			}
		}
		BehaviorTreeComponent->GetBlackboardComponent()->SetValue<UBlackboardKeyType_Object>(TargetKey.GetSelectedKeyID(),Target); 
	}
	
}

void UBTService_Enemy::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	if (UBlackboardData* BBData = GetBlackboardAsset())
	{
		TargetKey.ResolveSelectedKey(*BBData);
	}
	
	
}

void UBTService_Enemy::OnEquipWeapon(int32 ID)
{
	if (BehaviorTreeComponent)
	{
		BehaviorTreeComponent->GetBlackboardComponent()->SetValueAsBool(TEXT("bHoldWeapon"), true);
	}
	
}
