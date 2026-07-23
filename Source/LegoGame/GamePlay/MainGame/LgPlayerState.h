// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "LgPlayerState.generated.h"

enum class EJobType : uint8;
enum class ETeamType : uint8;
/**
 * 
 */
UCLASS()
class LEGOGAME_API ALgPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	
	void SetPlayerInfo(const FString& InPlayerName, ETeamType InTeamType, EJobType InJobType);
	
	ETeamType GetTeamType() const {return TeamType;}
	EJobType GetJobType() const {return JobType;}
	
protected:
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION()
	void OnRep_PlayerJobInfo();
	
protected:
	
	UPROPERTY(ReplicatedUsing=OnRep_PlayerJobInfo)
	FString PlayerName;
	UPROPERTY(ReplicatedUsing=OnRep_PlayerJobInfo)
	ETeamType TeamType;
	UPROPERTY(ReplicatedUsing=OnRep_PlayerJobInfo)
	EJobType JobType;
};
