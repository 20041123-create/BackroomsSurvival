// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_Enemy.generated.h"

class ALgCharacterBase;
/**
 * 
 */
UCLASS()
class LEGOGAME_API UBTService_Enemy : public UBTService
{
	GENERATED_BODY()
	UBTService_Enemy();
	
protected:
	
	//当开启了创建独立节点，可以重写节点创建函数
	virtual void OnInstanceCreated(UBehaviorTreeComponent& OwnerComp) override;
	virtual void OnInstanceDestroyed(UBehaviorTreeComponent& OwnerComp) override;
	
	void OnReceiveDamage(ALgCharacterBase* Target);
	
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	
	
	void OnEquipWeapon(int32 ID);
	
protected:
	UPROPERTY()
	TObjectPtr<UBehaviorTreeComponent> BehaviorTreeComponent;
	UPROPERTY(EditAnywhere)
	FBlackboardKeySelector TargetKey;
};
