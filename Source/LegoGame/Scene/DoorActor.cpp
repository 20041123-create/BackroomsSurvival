// Fill out your copyright notice in the Description page of Project Settings.


#include "DoorActor.h"

#include "Components/BoxComponent.h"
#include "LegoGame/Character/LgCharacterBase.h"


// Sets default values
ADoorActor::ADoorActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	StaticMeshComponent->SetupAttachment(RootComponent);
	
	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	BoxComponent->SetupAttachment(RootComponent);
	BoxComponent->SetCollisionProfileName(TEXT("Overlap"));
	
}

// Called when the game starts or when spawned
void ADoorActor::BeginPlay()
{
	Super::BeginPlay();
	BoxComponent->OnComponentBeginOverlap.AddDynamic(this,&ThisClass::OnBoxComponentBeginOverlapEvent);
	BoxComponent->OnComponentEndOverlap.AddDynamic(this,&ThisClass::OnBoxComponentEndOverlapEvent);
}

// Called every frame
void ADoorActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateDoorAnimation(DeltaTime);
}

void ADoorActor::UpdateDoorAnimation(float DeltaTime)
{
	if (!AnimeCurve || !bDoorAnimation)
	{
		return;
	}
	CurrentTimeTotal+=(DeltaTime * (bOpenDoor ? 1.0f : -1.0f));
	
	//然后处理从曲线上获取数据
	float Value = AnimeCurve->GetFloatValue(CurrentTimeTotal);
	StaticMeshComponent->SetRelativeRotation(FRotator(0,Value*90*OpenDirection,0));
	
	float Min = 0;
	float Max = 0;
	AnimeCurve->GetTimeRange(Min,Max);
	if (CurrentTimeTotal>=Max||CurrentTimeTotal<=Min)
	{
		bDoorAnimation = false;
		CurrentTimeTotal = FMath::Clamp(CurrentTimeTotal,Min,Max);
	}
	
}

void ADoorActor::OnBoxComponentBeginOverlapEvent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Cast<ALgCharacterBase>(OtherActor))
	{
		//计算开门方向
		FVector StandDirection = OtherActor->GetActorLocation()-GetActorLocation();
		//转为单位向量
		StandDirection.Normalize();
		//计算方向点乘
		float CosValue = FVector::DotProduct(StandDirection,GetActorRightVector());//夹角余弦值
		//根据余弦波范围判断开门方向
		if (CosValue>0)//站在们右侧
		{
			OpenDirection = -1;
		}
		else
		{
			OpenDirection = 1;
		}
		//处理问题
		bDoorAnimation = true;
		bOpenDoor = true;
	}
	
}

void ADoorActor::OnBoxComponentEndOverlapEvent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (Cast<ALgCharacterBase>(OtherActor))
	{
		//处理问题
		bDoorAnimation = true;
		bOpenDoor = false;
	}
	
}



