// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoginUserWidget.generated.h"

/**
 * 
 */
class UAccountSaveGame;
class UCheckBox;
class UButton;
class UEditableTextBox;

UCLASS()
class LEGOGAME_API ULoginUserWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	
	virtual void NativeOnInitialized() override;
	UFUNCTION()
	void OnShowPasswordPressed();
	UFUNCTION()
	void OnHidePasswordReleased();
	UFUNCTION(BlueprintCallable)
	void LoginGame();
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UEditableTextBox> AccountTextBox;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UEditableTextBox> PasswordTextBox;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> ShowPasswordButten;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCheckBox> PasswordCheckBox;
	UPROPERTY()
	TObjectPtr<UAccountSaveGame> AccountSaveGame;
};
