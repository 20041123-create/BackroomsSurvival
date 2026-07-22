// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DoorActor.generated.h"


class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class LEGOGAME_API ADoorActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ADoorActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnBoxComponentBeginOverlapEvent(UPrimitiveComponent* OverlappedComponent,
	                                  AActor* OtherActor,
	                                  UPrimitiveComponent* OtherComp,
	                                  int32 OtherBodyIndex,
	                                  bool bFromSweep,
	                                  const FHitResult& SweepResult);

	UFUNCTION()
	void OnBoxComponentEndOverlapEvent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                                UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	void UpdateDoorAnimation(float DeltaTime);
	
protected:
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> BoxComponent;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UCurveFloat> AnimeCurve; 
	//记录曲线的推动时间
	float CurrentTimeTotal;
	
	bool bDoorAnimation;
	bool bOpenDoor;
	
	float OpenDirection;
};
