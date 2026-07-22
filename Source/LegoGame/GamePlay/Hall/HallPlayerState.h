// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "HallPlayerState.generated.h"

enum class EChatChannel : uint8;
enum class EJobType : uint8;
enum class ETeamType : uint8;

DECLARE_MULTICAST_DELEGATE_FourParams(PlayerInfoChanged, int32, const FString& ,ETeamType,EJobType);
DECLARE_MULTICAST_DELEGATE_OneParam(PlayerReadyChanged, bool);
DECLARE_MULTICAST_DELEGATE(NotifyMaster);
/**
 * 
 */
UCLASS()
class LEGOGAME_API AHallPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	
	UFUNCTION(BlueprintCallable)
	void SetPlayerInfo(int32 InHeadIndex,const FString& InHallPlayerName,ETeamType InTeamType,EJobType InJobType);
	
	ETeamType GetTeamType() const {return TeamType;}
	
	FString GetHallPlayerName() const {return HallPlayerName;}
	
	void SendChatMessageToSelf(EChatChannel Channel, const FText& Text);
	
	bool IsReady() const {return bReady;}
	void SetReady(bool bNewReady);
	
	bool IsMaster() const {return bMaster;}
	
protected:
	
	virtual void BeginPlay() override;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void CopyProperties(APlayerState* PlayerState) override;
	
	UFUNCTION()
	void OnRep_PlayerInfoChanged();
	UFUNCTION()
	void OnRep_bReady();
	UFUNCTION()
	void OnRep_bMaster();
	//RPC
	UFUNCTION(Server,UnReliable,WithValidation)
	void Server_SetPlayerInfo(int32 InHeadIndex,const FString& InHallPlayerName,ETeamType InTeamType,EJobType InJobType);
	
	UFUNCTION(Client,Reliable)
	void Client_SendChatMessageToSelf(EChatChannel Channel, const FText& Text);
	
	UFUNCTION(Server,UnReliable,WithValidation)
	void Server_SetReady(bool bNewReady);
	
public:
	
	PlayerInfoChanged OnPlayerInfoChanged;
	PlayerReadyChanged OnReadyChanged;
	NotifyMaster OnIsMaster;
	
protected:
	
	UPROPERTY(ReplicatedUsing=OnRep_PlayerInfoChanged)
	int32 HeadIndex;
	UPROPERTY(ReplicatedUsing=OnRep_PlayerInfoChanged)
	FString HallPlayerName;
	UPROPERTY(ReplicatedUsing=OnRep_PlayerInfoChanged)
	ETeamType TeamType;
	UPROPERTY(ReplicatedUsing=OnRep_PlayerInfoChanged)
	EJobType JobType;
	UPROPERTY(ReplicatedUsing=OnRep_bReady)
	bool bReady;
	UPROPERTY(ReplicatedUsing=OnRep_bMaster)
	bool bMaster;
};
