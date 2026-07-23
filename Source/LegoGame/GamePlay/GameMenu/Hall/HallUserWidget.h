// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HallUserWidget.generated.h"

class UButton;
class UTextBlock;
enum class EChatChannel : uint8;
enum class ETeamType : uint8;
class UHallUserInfoWidget;
class UScrollBox;
/**
 * 
 */
UCLASS()
class LEGOGAME_API UHallUserWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UScrollBox* GetScrollBox(ETeamType TeamType);
	
	void PushMessage(EChatChannel Channel, const FText& Text);
	
protected:
	
	virtual void NativeOnInitialized() override;
	
	void OnAddPlayerState(APlayerState* PlayerState);
	void OnRemovePlayerState(APlayerState* PlayerState);
	
	UFUNCTION(BlueprintCallable)
	void SendChatMessage(EChatChannel ChatChannel, const FText& Text);
	
	UFUNCTION()
	void OnSubmitButtonClicked();
	
	void OnReadyChanged(bool bReady);
	void OnBecameMaster();
	
	UFUNCTION()
	void OnQuitButtonClicked();
	
	void QuitRoom();
	void EndGame();
	
protected:
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UScrollBox> PoliceScrollBox;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UScrollBox> BanditScrollBox;
	
	UPROPERTY()
	TSubclassOf<UHallUserInfoWidget> HallUserInfoWidget;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UScrollBox> ChatScrollBox;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ButtonTextBlock;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> SubmitButton;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> QuitButtonTextBlock;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> QuitButton;
};
