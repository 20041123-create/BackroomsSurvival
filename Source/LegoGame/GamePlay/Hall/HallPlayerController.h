// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "HallPlayerController.generated.h"

enum class EChatChannel : uint8;
/**
 * 
 */
UCLASS()
class LEGOGAME_API AHallPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	
	void SendChatMessage(EChatChannel Channel, const FText& Text);
	
protected:
	
	virtual void ClientWasKicked_Implementation(const FText& KickReason) override;
	virtual void ClientReturnToMainMenuWithTextReason_Implementation(const FText& ReturnReason) override;
	
	//RPC
	UFUNCTION(Server, Unreliable, WithValidation)
	void Server_SendChatMessage(EChatChannel Channel, const FText& Text);
};
