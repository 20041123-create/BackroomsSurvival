// Fill out your copyright notice in the Description page of Project Settings.


#include "LgCharacterMovementComponent.h"

#include "LegoGame/Character/LgCharacterBase.h"


// Sets default values for this component's properties
ULgCharacterMovementComponent::ULgCharacterMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	MaxWalkSpeed = 346.6f;
	SprintSpeed = 630.445f;
	MaxWalkSpeedCrouched = 90.831f;
	// ...
}


// Called when the game starts
void ULgCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

float ULgCharacterMovementComponent::GetMaxSpeed() const
{
	float Speed = Super::GetMaxSpeed();
	//添加我们的速度
	//获取组件的角色
	if (ALgCharacterBase* Character = Cast<ALgCharacterBase>(GetOwner()))
	{
		if (Character->IsSprinting()&&!Character->IsCrouched())
		{
			Speed = SprintSpeed;
		}
	}
	
	return Speed;
}


// Called every frame
void ULgCharacterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                  FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// ...
}

