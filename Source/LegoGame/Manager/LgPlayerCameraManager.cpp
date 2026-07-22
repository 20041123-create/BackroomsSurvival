// Fill out your copyright notice in the Description page of Project Settings.


#include "LgPlayerCameraManager.h"

#include "LegoGame/Character/LgCharacterBase.h"
#include "LegoGame/Player/PlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

void ALgPlayerCameraManager::UpdateCamera(float DeltaTime)
{
	//先修改位置在执行父节点update函数
	//与上一帧的逻辑比较
	//什么情况说明当前情况蹲下
	if (!IsValid(PlayerCharacter))
	{
		PlayerCharacter = Cast<APlayerCharacter>(PCOwner?PCOwner->GetPawn():nullptr);
	}
	if (PlayerCharacter)
	{
		if (PlayerCharacter->IsCrouched()&&!LastCrouchState)//上一帧为假，当前帧为真，说明当前帧蹲下
		{
			SpringArmZ = 48.f;
		}
		else if (!PlayerCharacter->IsCrouched()&&LastCrouchState)//当前帧为假，上一帧为真，说明玩家在当前帧站起来了
		{
			SpringArmZ = -48.f;
		}
		//记录当前状态
		LastCrouchState = PlayerCharacter->IsCrouched();
		
		SpringArmZ = FMath::FInterpTo(SpringArmZ,0,DeltaTime,5);//与lerp一样，且保证每个终端都一样
		PlayerCharacter->GetSpringArm()->SetRelativeLocation(FVector(0,0,SpringArmZ));
		
		//拿枪时调整吊臂位置
		float SpringSocketOffsetY = 0;
		float CameraX = 0;
		float TargetFOV = 90;
		if (PlayerCharacter->IsIronSight())
		{
			SpringSocketOffsetY = 60;
			CameraX = 150;
			TargetFOV = 60;
		}
		//计算，通过插值完成运算
		PlayerCharacter->GetSpringArm()->SocketOffset.Y = FMath::FInterpTo(PlayerCharacter->GetSpringArm()->SocketOffset.Y,SpringSocketOffsetY,DeltaTime,10);
		
		//调整相机位置
		float Cx = FMath::FInterpTo(PlayerCharacter->GetCameraComponent()->GetRelativeLocation().X,CameraX,DeltaTime,10);
		PlayerCharacter->GetCameraComponent()->SetRelativeLocation(FVector(Cx,0,0));
		
		//调整FOV
		DefaultFOV = FMath::FInterpTo(DefaultFOV,TargetFOV,DeltaTime,10);
		SetFOV(DefaultFOV);
	}
	
	
	
	Super::UpdateCamera(DeltaTime);
}
