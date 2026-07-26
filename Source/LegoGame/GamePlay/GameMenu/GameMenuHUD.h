// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GameMenuHUD.generated.h"


class UGameMenuUserWidget;
class URegisterUserWidget;
class ULoginUserWidget;
class USettingUserWidget;//前向声明
/**
 * 
 */
UCLASS()
class LEGOGAME_API AGameMenuHUD : public AHUD
{
	GENERATED_BODY()
public:
	TObjectPtr<USettingUserWidget> GetSettingUserWidget() const{return SettingUserWidget;}

	
protected:
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable)
	void ShowSettingUI();
	
	UFUNCTION(BlueprintCallable)
	void ShowLoginUI();
	
	UFUNCTION(BlueprintCallable)
	void ShowRegisterUI();
	
	UFUNCTION(BlueprintCallable)
	void ShowRoomListUI();
	
	UFUNCTION(BlueprintCallable)
	void ShowCreateRoomUI();
	
	UFUNCTION(BlueprintCallable)
	void ShowWaitingUI();
	
	UFUNCTION(BlueprintCallable)
	void ShowGameMenuUI();
	
	UFUNCTION(BlueprintCallable, Category = "UI")
	UUserWidget* GetWaitingUI() const{return WaitingWidget;}
	
protected:
	UPROPERTY()
	TObjectPtr<UGameMenuUserWidget> GameMenuUserWidget;
	
	UPROPERTY()
	TObjectPtr<USettingUserWidget> SettingUserWidget;
	
	UPROPERTY()
	TObjectPtr<ULoginUserWidget> LoginUserWidget;
	
	UPROPERTY()
	TObjectPtr<URegisterUserWidget> RegisterUserWidget;

	UPROPERTY()
	TObjectPtr<UUserWidget> RoomListWidget;

	UPROPERTY()
	TObjectPtr<UUserWidget> CreateRoomWidget;

	UPROPERTY()
	TObjectPtr<UUserWidget> WaitingWidget; 
};



