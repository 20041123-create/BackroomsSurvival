// Fill out your copyright notice in the Description page of Project Settings.


#include "BTService_CivilianEnemy.h"

#include "AIController.h"
#include "LegoGame/LegoGame.h"
#include "LegoGame/Components/PackageComponent.h"
#include "LegoGame/Enemy/EnemyCharacter.h"
#include "LegoGame/Scene/SceneItemActor.h"

UBTService_CivilianEnemy::UBTService_CivilianEnemy()
{
	NodeName = TEXT("Service_CivilianEnemy");
	
}

void UBTService_CivilianEnemy::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	//扫描附近有没有道具，如果有道具直接穿上
	//物理帧检测
	//构建检测形状
	FCollisionShape CollisionShape;
	CollisionShape.ShapeType = ECollisionShape::Sphere;
	//设置检测半径
	CollisionShape.Sphere.Radius = 150.f;
	//检测原点
	const FVector StandLocation = OwnerComp.GetAIOwner()->GetPawn()->GetActorLocation();
	//创建询问参数
	FCollisionObjectQueryParams CollisionParams;
	CollisionParams.AddObjectTypesToQuery(ECC_GameTraceChannel1);
	
	//发起检测
	FHitResult HitResult;//检测结果
	//通过物体类型进行检测
	if (OwnerComp.GetWorld()->SweepSingleByObjectType(HitResult,StandLocation,StandLocation,FRotator::ZeroRotator.Quaternion(),CollisionParams,CollisionShape))
	{
		//UE_LOG(LogTemp, Warning, TEXT("%s"),*HitResult.GetActor()->GetName());
		if (ASceneItemActor* ItemActor = Cast<ASceneItemActor>(HitResult.GetActor()))
		{
			if (AEnemyCharacter* EnemyCharacter = Cast<AEnemyCharacter>(OwnerComp.GetAIOwner()->GetPawn()))
			{
				if (EnemyCharacter->GetPackageComponent())
				{
					//装备和穿戴
					if (ItemActor->GetID()<WEAPON_INDEX)
					{
						//穿戴
						EnemyCharacter->GetPackageComponent()->PutOnSkinFromNear(ItemActor);
					}
					else
					{
						EnemyCharacter->GetPackageComponent()->EquipWeaponFromNear(ItemActor);
					}
				}
			}
		}
	}
	
	//绘制调试框
	DrawDebugSphere(OwnerComp.GetAIOwner()->GetWorld(),StandLocation,150.f,10,FColor::Red,false,3);
}
