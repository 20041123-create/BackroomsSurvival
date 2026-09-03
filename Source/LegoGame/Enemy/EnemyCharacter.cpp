// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacter.h"

#include "EnemyController.h"
#include "LegoGame/Survival/SurvivalVitalsComponent.h"


// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	
	//修改控制器
	AIControllerClass = AEnemyController::StaticClass();
	//更换控制器的启用模式
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

bool AEnemyCharacter::InitializeSurvivalDifficulty(float DifficultyMultiplier)
{
	if (!HasAuthority() || HasActorBegunPlay() || !FMath::IsFinite(DifficultyMultiplier) || DifficultyMultiplier <= 0.0f)
	{
		return false;
	}

	USurvivalVitalsComponent* Vitals = GetSurvivalVitalsComponent();
	return Vitals && Vitals->SetInitialDifficultyMultiplier(DifficultyMultiplier);
}

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

