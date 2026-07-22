// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LegoGame/Interface/SkinInterface.h"
#include "PlayerModelActor.generated.h"


class APlayerCharacter;
class USkinComponent;
class USkeletalMeshComponent;

UCLASS()
class LEGOGAME_API APlayerModelActor : public AActor,public ISkinInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APlayerModelActor();

	void SetBindPlayer(APlayerCharacter* PlayerCharacter);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual USkeletalMeshComponent* GetSkeletalMeshComponent() override {return SkeletalMeshComponent;}

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	
protected:
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;
	UPROPERTY(VisibleAnywhere,BlueprintReadWrite)
	TObjectPtr<USceneCaptureComponent2D> SceneCaptureComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkinComponent> SkinComponent; 
	
};
