// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "LgAnimInstance.generated.h"

class ALgCharacterBase;
/**
 * 
 */
UCLASS()
class LEGOGAME_API ULgAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	
	ULgAnimInstance();
	
protected:
	
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
	virtual void NativeInitializeAnimation() override;
	
	UFUNCTION()
	void AnimNotify_RelaxedEnd();
	
protected:
	
	UPROPERTY(BlueprintReadOnly)
	float Speed;
	
	UPROPERTY()
	TObjectPtr<ALgCharacterBase> OwnerCharacter;
	
	UPROPERTY(BlueprintReadOnly)
	bool bSprinting;
	UPROPERTY(BlueprintReadOnly)
	bool bIsCrouched;
	UPROPERTY(BlueprintReadOnly)
	bool bRelaxed;
	UPROPERTY(BlueprintReadOnly)
	bool bHoldWeapon;
	UPROPERTY(BlueprintReadOnly)
	bool bIronSight;
	
	UPROPERTY(EditAnywhere)
	float WaitRelaxedTime;
	float RelaxedTick;//记录进入relaxed的剩余时间
	
	UPROPERTY(BlueprintReadOnly)
	float Direction;
	
	UPROPERTY(BlueprintReadOnly)
	float AimPitch;
	
};
