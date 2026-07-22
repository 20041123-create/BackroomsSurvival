// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneItemActor.h"

#include "Components/BillboardComponent.h"
#include "LegoGame/Subsystem/PropsSubsystem.h"
#include "Net/UnrealNetwork.h"


// Sets default values
ASceneItemActor::ASceneItemActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("BillBoardComponent"));
	BillboardComponent->SetupAttachment(RootComponent);
	//开启网络同步
	bReplicates = true;
	
	ID = -1;
}

// Called when the game starts or when spawned
void ASceneItemActor::BeginPlay()
{
	Super::BeginPlay();
	InitMesh();
}

void ASceneItemActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION(ASceneItemActor,ID,COND_InitialOnly);
}

void ASceneItemActor::InitMesh()
{
	if (ID<0)//说明当前场景道具没有有效的道具ID
	{
		return;
	}
	//找到道具管理器，读取道具数据
	const FPropsBase* Props = GetWorld()->GetGameInstance()->GetSubsystem<UPropsSubsystem>()->GetPropsById(ID);
	if (!Props)
	{
		return;
	}
	//UE_LOG(LogTemp, Warning, TEXT("%s"),*Props->Name.ToString());
	//如果是装饰道具就转换到装饰数据表头
	if (Props->Type == EPropsType::EPT_Skin)
	{
		//转换类型
		const FSkinHeader* SkinHeader = static_cast<const FSkinHeader*>(Props);//转换的指针是继承的，所以使用static_cast
		if (SkinHeader->StaticMesh)
		{
			//动态创建组件
			StaticMeshComponent = NewObject<UStaticMeshComponent>(this);
			//注册到世界
			StaticMeshComponent->RegisterComponentWithWorld(GetWorld());
			//napToTargetNotIncludingScale将组件放到父节点的0，0，0坐标点
			StaticMeshComponent->AttachToComponent(RootComponent,FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			//设置碰撞预设
			StaticMeshComponent->SetCollisionProfileName(TEXT("SceneItemProfile"));
			//设置资产
			StaticMeshComponent->SetStaticMesh(SkinHeader->StaticMesh);
		}
		else if (SkinHeader->SkeletalMesh)
		{
			SkeletalMeshComponent = NewObject<USkeletalMeshComponent>(this);
			SkeletalMeshComponent->RegisterComponentWithWorld(GetWorld());
			SkeletalMeshComponent->AttachToComponent(RootComponent,FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			SkeletalMeshComponent->SetCollisionProfileName(TEXT("SceneItemProfile"));
			SkeletalMeshComponent->SetSkeletalMesh(SkinHeader->SkeletalMesh);
			//开启生成堆叠事件
			SkeletalMeshComponent->SetGenerateOverlapEvents(true);
		}
		
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("weapon"));
		const FWeaponBaseHeader* WeaponBaseHeader = static_cast<const FWeaponBaseHeader*>(Props);
		SkeletalMeshComponent = NewObject<USkeletalMeshComponent>(this);
		SkeletalMeshComponent->RegisterComponentWithWorld(GetWorld());
		SkeletalMeshComponent->AttachToComponent(RootComponent,FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		SkeletalMeshComponent->SetCollisionProfileName(TEXT("SceneItemProfile"));
		SkeletalMeshComponent->SetSkeletalMesh(WeaponBaseHeader->SkeletalMesh);
		//开启生成堆叠事件
		SkeletalMeshComponent->SetGenerateOverlapEvents(true);
			
	}
}

// Called every frame
void ASceneItemActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

