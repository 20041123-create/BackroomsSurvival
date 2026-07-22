// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LegoGame/Character/LgCharacterBase.h"
#include "PlayerCharacter.generated.h"

class USphereComponent;
struct FInputActionValue;
class UCustomKeySaveGame;
class UInputAction;
class UCameraComponent;
class USpringArmComponent;
class UInputMappingContext;

UCLASS()
class LEGOGAME_API APlayerCharacter : public ALgCharacterBase
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APlayerCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	void SetUpPlayerInputMappingContext();
	FKey GetUserCustomKey(FName KeyEventName);
	UInputAction* GetInputAction(FName KeyEventName);
	
	void Move(FInputActionValue const& InputActionValue);
	
	void AddAxisActionKey(FName KeyEventName, EAxis::Type Axis, bool bNegate = false);
	
	void Look(FInputActionValue const& InputActionValue); 
	
	void TogglePackageUI();

	UFUNCTION()
	void OnComponentBeginOverlapEvent(UPrimitiveComponent* OverlappedComponent,
	                                  AActor* OtherActor,
	                                  UPrimitiveComponent* OtherComp,
	                                  int32 OtherBodyIndex,
	                                  bool bFromSweep,
	                                  const FHitResult& SweepResult);
	UFUNCTION()
	void OnComponentEndOverlapEvent(UPrimitiveComponent* OverlappedComponent, 
									 AActor* OtherActor, 
									 UPrimitiveComponent* OtherComp, 
									 int32 OtherBodyIndex);
	
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	USpringArmComponent* GetSpringArm()const {return SpringArmComponent;}
	
	TObjectPtr<UCameraComponent> GetCameraComponent()const {return CameraComponent;} 
	
protected:
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USpringArmComponent> SpringArmComponent;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> CameraComponent;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> SphereComponent;
	// UPROPERTY(EditAnywhere)
	// TObjectPtr<UInputAction> IA_Move;
	// UPROPERTY(EditAnywhere)
	// TObjectPtr<UInputAction> IA_Jump;
	// UPROPERTY(EditAnywhere)
	// TObjectPtr<UInputAction> IA_Fire;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UInputMappingContext> IMC_Player;
	
	UPROPERTY()
	TObjectPtr<UDataTable> DT_KeyMapping; 
	UPROPERTY()
	TObjectPtr<UCustomKeySaveGame> CustomKeySaveGame;
	
	float MouseSpeed = 0;
};
