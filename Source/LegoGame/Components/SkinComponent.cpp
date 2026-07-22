// Fill out your copyright notice in the Description page of Project Settings.


#include "SkinComponent.h"

#include "LegoGame/Character/LgCharacterBase.h"
#include "LegoGame/Interface/SkinInterface.h"
#include "LegoGame/Subsystem/PropsSubsystem.h"



// Sets default values for this component's properties
USkinComponent::USkinComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

void USkinComponent::OnPutOnSkin(ESkinType SkinType, int32 ID)
{
	const FPropsBase* PropsBase = GetWorld()->GetGameInstance()->GetSubsystem<UPropsSubsystem>()->GetPropsById(ID);
	if (!PropsBase||PropsBase->Type == EPropsType::EPT_Weapon)
	{
		return;
	}
	const FSkinHeader* SkinHeader = static_cast<const FSkinHeader*>(PropsBase);
	if (SkinHeader->StaticMesh)
	{
		GetStaticMeshComponent(SkinType)->SetStaticMesh(SkinHeader->StaticMesh);
	}
	else
	{
		GetSkeletalMeshComponent()->SetSkeletalMesh(SkinHeader->SkeletalMesh);
	}
}

void USkinComponent::OnTakeOffSkin(ESkinType SkinType, int32 ID)
{
	if (SkinType!=ESkinType::EST_Clothes)
	{
		if (SkinMeshComponentMap.Contains(SkinType))
		{
			SkinMeshComponentMap[SkinType]->SetStaticMesh(nullptr);
		}
	}
	else
	{
		GetSkeletalMeshComponent()->SetSkeletalMesh(nullptr);
	}
}

FName USkinComponent::GetSocketName(ESkinType SkinType)
{
	if (SkinType == ESkinType::EST_Package)
	{
		return TEXT("backSocket");
	}
	return TEXT("headSocket");
}


UStaticMeshComponent* USkinComponent::GetStaticMeshComponent(ESkinType SkinType)
{
	if (SkinMeshComponentMap.Contains(SkinType))
	{
		return SkinMeshComponentMap[SkinType];
	}
	//没有就创建动态网格组件
	UStaticMeshComponent* StaticMeshComponent = NewObject<UStaticMeshComponent>(GetOwner());
	//注册到世界
	StaticMeshComponent->RegisterComponentWithWorld(GetWorld());
	//设置依附关系
	if (ISkinInterface* Interface = Cast<ISkinInterface>(GetOwner()))
	{
		StaticMeshComponent->AttachToComponent(Interface->GetSkeletalMeshComponent(),FAttachmentTransformRules::SnapToTargetNotIncludingScale,GetSocketName(SkinType));
	}
	//关闭碰撞
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkinMeshComponentMap.Add(SkinType, StaticMeshComponent);
	return StaticMeshComponent;
}

USkeletalMeshComponent* USkinComponent::GetSkeletalMeshComponent()
{
	if (!SkeletalMeshComponent)
	{
		SkeletalMeshComponent = NewObject<USkeletalMeshComponent>(GetOwner());
		SkeletalMeshComponent->RegisterComponentWithWorld(GetWorld());
		if (ISkinInterface* Interface = Cast<ISkinInterface>(GetOwner()))
		{
			SkeletalMeshComponent->AttachToComponent(Interface->GetSkeletalMeshComponent(),FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			//如何让服装的骨架跟随玩家的骨架进行动画同步
			//前提是：两套骨架网格绑定的骨架一致
			SkeletalMeshComponent->SetLeaderPoseComponent(Interface->GetSkeletalMeshComponent());
		}
		SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	return SkeletalMeshComponent;
}




