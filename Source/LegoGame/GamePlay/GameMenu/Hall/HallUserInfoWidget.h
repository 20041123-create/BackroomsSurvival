// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HallUserInfoWidget.generated.h"

class AHallPlayerState;
class UButton;
class UBorder;
class UTextBlock;
enum class EJobType : uint8;
enum class ETeamType : uint8;
/**
 * 
 */
UCLASS()
class LEGOGAME_API UHallUserInfoWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	void BindPlayerState(APlayerState* PlayerState);
	
	AHallPlayerState* GetHallPlayerState() const {return BindHallPlayerState;}
	
protected:
	
	void OnPlayerInfoChanged(int32 HeadIndex, const FString& PlayerName, ETeamType TeamType, EJobType JobType);
	void OnPlayerReadyChanged(bool bReady);
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnHeadIndexChanged(int32 HeadIndex);
	
	void OnMaster();
	UFUNCTION()
	void OnKickPlayer();
	
protected:
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> PlayerNameTextBlock; 
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> JobTextBlock; 
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UBorder> BgBorder; 
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> KickButton;

	UPROPERTY()
	TObjectPtr<AHallPlayerState> BindHallPlayerState;
};
