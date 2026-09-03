// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LegoGame/Character/LgCharacterBase.h"
#include "EnemyCharacter.generated.h"

class UBehaviorTree;

UCLASS()
class LEGOGAME_API AEnemyCharacter : public ALgCharacterBase
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyCharacter();

	/** Server-only deferred-spawn setup. The multiplier is applied once before BeginPlay. */
	bool InitializeSurvivalDifficulty(float DifficultyMultiplier);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	TObjectPtr<UBehaviorTree> GetBehaviorTree() const {return BehaviorTree;}
	
	
protected:
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBehaviorTree> BehaviorTree; 
	
};
