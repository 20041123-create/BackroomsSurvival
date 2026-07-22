// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "LgPlayerStart.generated.h"

enum class ETeamType : uint8;

UCLASS()
class LEGOGAME_API ALgPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ALgPlayerStart(const FObjectInitializer& ObjectInitializer);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	ETeamType GetTeamType() const {return TeamType;}
	
protected:
	UPROPERTY(EditAnywhere)
	ETeamType TeamType;
};
