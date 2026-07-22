// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/HUD.h"
#include "LgHUD.generated.h"

class UGameFetureUserWidget;
class APlayerCharacter;
class UUPackageUserWidget;
/**
 * 
 */
UCLASS()
class LEGOGAME_API ALgHUD : public AHUD
{
	GENERATED_BODY()
public:
	void TogglePackageUI();
	
	template<typename T>
	T* GetSingleWidgetObject(T* ObjectTemplate);

protected:
	
	virtual void BeginPlay() override;
	
	virtual void DrawHUD() override;
	
	
protected:
	
	UPROPERTY()
	TObjectPtr<UUPackageUserWidget> PackageUserWidget;
	
	UPROPERTY()
	TObjectPtr<APlayerCharacter> PlayerCharacter;

	UPROPERTY()
	TObjectPtr<UGameFetureUserWidget> GameFetureUserWidget; 
	
	UPROPERTY()
	TMap<UClass*,UUserWidget*> SingleObjectMap;
};

template <typename T>
T* ALgHUD::GetSingleWidgetObject(T* ObjectTemplate)
{
	if (SingleObjectMap.Contains(T::StaticClass()))
	{
		return Cast<T>(SingleObjectMap[T::StaticClass()]);
	}
	T* Object = CreateWidget<T>(GetOwningPlayerController(), ObjectTemplate->GetClass());
	
	SingleObjectMap.Add(T::StaticClass(), Object);
	return Object;
	
}