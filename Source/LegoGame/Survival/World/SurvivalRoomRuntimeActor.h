#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LegoGame/Survival/Contracts/SurvivalInterfaces.h"
#include "SurvivalLayoutPlanner.h"
#include "SurvivalRoomRuntimeActor.generated.h"

class UStaticMesh;
class UBoxComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class URectLightComponent;

UCLASS(BlueprintType)
class LEGOGAME_API ASurvivalRoomRuntimeActor : public AActor, public ISurvivalRoomRuntimeInterface
{
	GENERATED_BODY()

public:
	ASurvivalRoomRuntimeActor();

	void InitializeRoom(const LG::Survival::World::FPlannedRoom& InPlan, float InCellSize);
	/** Re-registers generated collision with the authority navigation octree. */
	void RefreshNavigationRegistration();

	virtual FRoomHandle GetRoomHandle_Implementation() const override;
	virtual FGameplayTag GetRoomTypeTag_Implementation() const override;
	virtual void SetRoomState_Implementation(ESurvivalRoomState NewState) override;

	UFUNCTION(BlueprintPure, Category="Survival|Room")
	int32 GetUnlockPhaseIndex() const { return UnlockPhaseIndex; }

	UFUNCTION(BlueprintPure, Category="Survival|Room")
	FName GetTemplateId() const { return TemplateId; }

	const TArray<FRoomConnectorDefinition>& GetRuntimeConnectors() const { return RuntimeConnectors; }
	FIntPoint GetFootprint() const { return Footprint; }
	float GetCellSize() const { return CellSize; }

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_RoomLayout();

	UFUNCTION()
	void OnRep_RoomState();

	void RebuildGeometry();
	bool HasConnectorAt(const FIntPoint Cell, ERoomConnectorDirection Direction) const;
	UInstancedStaticMeshComponent* ConfigureGeometryBatch(UInstancedStaticMeshComponent* Component, UStaticMesh* Mesh, ECollisionEnabled::Type CollisionMode, bool bAffectsNavigation);
	FTransform MakeBoundsCenteredTransform(const UStaticMesh* Mesh, const FVector& BoundsCenter, const FRotator& Rotation = FRotator::ZeroRotator, const FVector& Scale = FVector::OneVector) const;
	UMaterialInterface* ResolveCompatibleMaterial(UMaterialInterface* SourceMaterial) const;
	void AddReferenceLight(const FVector& RelativeLocation, float IntensityScale, const FLinearColor& LightColor);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	/** Stable simple floor collision used by character movement and runtime navigation generation. */
	UPROPERTY(VisibleAnywhere, Category="Survival|Room")
	TObjectPtr<UBoxComponent> NavigationFloor;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> BlockMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> FloorMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> WallMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CeilingMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> LampCeilingMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> LampMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> PillarMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> VentMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> CarpetMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> LinoleumMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> CeilingPanelsMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> CeilingFrameMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> LampMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> WallpaperMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PlasterMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PlasterWhiteMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> WallTrimMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> WallTrimDarkMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> CeilingVentMaterial;

	// Fixed default subobjects give character movement a stable, network-supported MovementBase.
	UPROPERTY(VisibleAnywhere, Category="Survival|Room")
	TObjectPtr<UInstancedStaticMeshComponent> FloorGeometry;

	UPROPERTY(VisibleAnywhere, Category="Survival|Room")
	TObjectPtr<UInstancedStaticMeshComponent> CeilingGeometry;

	UPROPERTY(VisibleAnywhere, Category="Survival|Room")
	TObjectPtr<UInstancedStaticMeshComponent> LampCeilingGeometry;

	UPROPERTY(VisibleAnywhere, Category="Survival|Room")
	TObjectPtr<UInstancedStaticMeshComponent> WallGeometry;

	UPROPERTY(VisibleAnywhere, Category="Survival|Room")
	TObjectPtr<UInstancedStaticMeshComponent> LampGeometry;

	UPROPERTY(VisibleAnywhere, Category="Survival|Room")
	TObjectPtr<UInstancedStaticMeshComponent> PillarGeometry;

	UPROPERTY(VisibleAnywhere, Category="Survival|Room")
	TObjectPtr<UInstancedStaticMeshComponent> VentGeometry;

	UPROPERTY(VisibleAnywhere, Category="Survival|Room")
	TObjectPtr<UInstancedStaticMeshComponent> CeilingBackingGeometry;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> GeneratedGeometry;

	UPROPERTY(Transient)
	TArray<TObjectPtr<URectLightComponent>> GeneratedLights;

	UPROPERTY(ReplicatedUsing=OnRep_RoomLayout, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Room")
	FRoomHandle RoomHandle;

	UPROPERTY(ReplicatedUsing=OnRep_RoomLayout, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Room")
	FName TemplateId = NAME_None;

	UPROPERTY(ReplicatedUsing=OnRep_RoomLayout, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Room")
	FGameplayTag RoomTypeTag;

	UPROPERTY(ReplicatedUsing=OnRep_RoomLayout, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Room")
	FIntPoint Footprint = FIntPoint(1, 1);

	UPROPERTY(ReplicatedUsing=OnRep_RoomLayout, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Room")
	TArray<FRoomConnectorDefinition> RuntimeConnectors;

	UPROPERTY(ReplicatedUsing=OnRep_RoomLayout, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Room")
	int32 RotationQuarterTurns = 0;

	UPROPERTY(ReplicatedUsing=OnRep_RoomLayout, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Room")
	int32 UnlockPhaseIndex = 0;

	UPROPERTY(ReplicatedUsing=OnRep_RoomLayout, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Room")
	float CellSize = 2000.0f;

	UPROPERTY(ReplicatedUsing=OnRep_RoomState, VisibleAnywhere, BlueprintReadOnly, Category="Survival|Room")
	ESurvivalRoomState RoomState = ESurvivalRoomState::Planned;
};
