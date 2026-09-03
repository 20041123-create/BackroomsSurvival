// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LegoGame/Survival/Contracts/SurvivalInterfaces.h"
#include "SceneItemActor.generated.h"


class UBillboardComponent;
UCLASS()
class LEGOGAME_API ASceneItemActor : public AActor, public ISurvivalInteractableInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASceneItemActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	void InitMesh();

	UFUNCTION()
	void OnRep_ItemStack();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	int32 GetID() const {return ItemStack.IsValid() ? ItemStack.ItemId : ID;}
	
	void SetID(int32 NewID);
	const FItemStack& GetItemStack() const { return ItemStack; }
	void SetItemStack(const FItemStack& NewItemStack);

	virtual bool CanInteract_Implementation(APawn* InstigatorPawn) const override;
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;

	
protected:
	//广告牌
	UPROPERTY(VisibleAnywhere)
	UBillboardComponent* BillboardComponent;
	
	UPROPERTY(EditAnywhere,Replicated)
	int32 ID;

	UPROPERTY(ReplicatedUsing=OnRep_ItemStack, EditAnywhere, Category="Survival")
	FItemStack ItemStack;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;
};
