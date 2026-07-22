// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerModelActor.h"

#include "PlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "LegoGame/Components/PackageComponent.h"
#include "LegoGame/Components/SkinComponent.h"


// Sets default values
APlayerModelActor::APlayerModelActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	
	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	SkeletalMeshComponent->SetupAttachment(RootComponent);
	
	SceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent"));
	SceneCaptureComponent->SetupAttachment(RootComponent);
	
	SkinComponent = CreateDefaultSubobject<USkinComponent>(TEXT("SkinComponent"));
}

void APlayerModelActor::SetBindPlayer(APlayerCharacter* PlayerCharacter)
{
	if (PlayerCharacter)
	{
		//绑定通知
		PlayerCharacter->GetPackageComponent()->OnPutOnSkin.AddUObject(SkinComponent,&USkinComponent::OnPutOnSkin);
		PlayerCharacter->GetPackageComponent()->OnTakeOffSkin.AddUObject(SkinComponent,&USkinComponent::OnTakeOffSkin);
	
		//同步角色网格资产
		SkeletalMeshComponent->SetSkeletalMesh(PlayerCharacter->GetMesh()->GetSkeletalMeshAsset());
	}
	
}

// Called when the game starts or when spawned
void APlayerModelActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APlayerModelActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

