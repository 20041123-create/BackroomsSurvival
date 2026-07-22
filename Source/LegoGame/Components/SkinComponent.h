// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkinComponent.generated.h"


enum class ESkinType : uint8;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LEGOGAME_API USkinComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USkinComponent();
	
	void OnPutOnSkin(ESkinType SkinType,int32 ID);
	void OnTakeOffSkin(ESkinType SkinType,int32 ID);

protected:
	
	FName GetSocketName(ESkinType SkinType);
	
	UStaticMeshComponent* GetStaticMeshComponent(ESkinType SkinType);
	
	USkeletalMeshComponent* GetSkeletalMeshComponent();

protected:
	
	UPROPERTY()
	TMap<ESkinType,UStaticMeshComponent*> SkinMeshComponentMap; 
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;
	
};
