// Fill out your copyright notice in the Description page of Project Settings.


#include "LgAnimInstance.h"

#include "LegoGame/Player/PlayerCharacter.h"

ULgAnimInstance::ULgAnimInstance()
{
	WaitRelaxedTime = 5.f;
}

void ULgAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	if ((RelaxedTick -= DeltaSeconds) <= 0.0f)//说明倒计时结束
	{
		bRelaxed = true;
	}
	
	//同步角色行为，然后用于更新动画状态
	
	//获取速度  获取当前使用这个动画的人的速度
	//TryGetPawnOwner();//使用这个动画蓝图的人的实例
	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ALgCharacterBase>(TryGetPawnOwner());
		return;
	}
	
	//获取速度
	Speed = OwnerCharacter->GetVelocity().Size2D();
	bSprinting = OwnerCharacter->IsSprinting() && Speed>0;
	bIsCrouched = OwnerCharacter->IsCrouched();
	bHoldWeapon = OwnerCharacter->GetHoldWeapon() != nullptr;
	
	if (bHoldWeapon)
	{
		//计算速度
		FVector MoveDirection = OwnerCharacter->GetVelocity().GetSafeNormal();
		//通过移动的方向和正方向进行点乘
		float CosValue = FVector::DotProduct(MoveDirection,OwnerCharacter->GetActorForwardVector());
		//反余弦求角度 (ACos返回的是弧度) RadiansToDegrees将弧度转化为角度
		Direction = FMath::RadiansToDegrees(FMath::Acos(CosValue));
		//因为左右走和正方向都是90°夹角，需要计算向左向右
		if (FVector::DotProduct(MoveDirection,OwnerCharacter->GetActorRightVector())<0)//小于0说明在左，点乘结果大于0说明方向相同
		{
			Direction *= -1;
		}
		
		bIronSight = OwnerCharacter->IsIronSight();
		
		AimPitch = OwnerCharacter->GetBaseAimRotation().GetNormalized().Pitch;
	}	
	else
	{
		if (bIsCrouched||Speed>0||OwnerCharacter->IsJumping())
		{
			bRelaxed = false;
			RelaxedTick = WaitRelaxedTime;
		}
	}
	
}

void ULgAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	RelaxedTick = WaitRelaxedTime;
}

void ULgAnimInstance::AnimNotify_RelaxedEnd()
{
	bRelaxed = false;
	RelaxedTick = WaitRelaxedTime;
}
