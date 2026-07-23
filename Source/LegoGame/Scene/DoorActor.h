#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DoorActor.generated.h"

class ALgCharacterBase;
class UBoxComponent;
class UCurveFloat;
class UStaticMeshComponent;

USTRUCT()
struct FDoorReplicatedState
{
	GENERATED_BODY()

	UPROPERTY()
	bool bOpen = false;

	UPROPERTY()
	float OpenDirection = 1.0f;

	UPROPERTY()
	float StartCurveTime = 0.0f;

	UPROPERTY()
	float StateChangeServerTime = 0.0f;
};

UCLASS()
class LEGOGAME_API ADoorActor : public AActor
{
	GENERATED_BODY()

public:
	ADoorActor();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnBoxComponentBeginOverlapEvent(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnBoxComponentEndOverlapEvent(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	void UpdateDoorAnimation(float DeltaTime);

protected:
	void SetDoorOpen(bool bOpen, float NewOpenDirection);
	float GetSynchronizedServerTime() const;

	UFUNCTION()
	void OnRep_DoorState();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> BoxComponent;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UCurveFloat> AnimeCurve;

	UPROPERTY(ReplicatedUsing=OnRep_DoorState)
	FDoorReplicatedState DoorState;

	TMap<TWeakObjectPtr<ALgCharacterBase>, int32> OverlappingCharacters;

	float CurrentTimeTotal = 0.0f;
	bool bDoorAnimation = false;
};
