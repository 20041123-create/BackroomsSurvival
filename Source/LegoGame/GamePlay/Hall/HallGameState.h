// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "HallGameState.generated.h"

enum class EChatChannel : uint8;
DECLARE_MULTICAST_DELEGATE_OneParam(HallPlayerStateDelegate, APlayerState*);

/**
 * 
 */
UCLASS()
class LEGOGAME_API AHallGameState : public AGameState
{
	GENERATED_BODY()
	
public:
	
	HallPlayerStateDelegate OnAddPlayerState;
	HallPlayerStateDelegate OnRemovePlayerState;
	
	void PushPublicMessage(EChatChannel Channel, const FText& Text);
	
protected:
	
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	
	//RPC
	UFUNCTION(NetMulticast,Reliable)
	void Multi_PushPublicMessage(EChatChannel Channel, const FText& Text);
};
