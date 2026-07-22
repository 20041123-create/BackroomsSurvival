// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SceneItemActor.generated.h"


class UBillboardComponent;
UCLASS()
class LEGOGAME_API ASceneItemActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASceneItemActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	void InitMesh();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	int32 GetID() const {return ID;}
	
	void SetID(int32 NewID) {ID = NewID;}

	
protected:
	//广告牌
	UPROPERTY(VisibleAnywhere)
	UBillboardComponent* BillboardComponent;
	
	UPROPERTY(EditAnywhere,Replicated)
	int32 ID;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;
};
