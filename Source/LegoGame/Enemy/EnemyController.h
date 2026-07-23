// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyController.generated.h"

struct FAIStimulus;

UCLASS()
class LEGOGAME_API AEnemyController : public AAIController
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AEnemyController();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void OnPossess(APawn* InPawn) override;
	
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	
	UFUNCTION()
	void OnTargetPerceptionUpdate(AActor* Actor, FAIStimulus Stimulus);

	void ForgetPendingSightTarget();

protected:
	UPROPERTY(EditDefaultsOnly, Category="AI|Perception", meta=(ClampMin="0.0"))
	float LostSightForgetDelay = 5.0f;

	TWeakObjectPtr<AActor> PendingLostTarget;
	FTimerHandle LostSightForgetTimer;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
