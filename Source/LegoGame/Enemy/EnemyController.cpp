// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyController.h"

#include "EnemyCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "LegoGame/LegoGame.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"


// Sets default values
AEnemyController::AEnemyController()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//添加感知组件
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	
}

// Called when the game starts or when spawned
void AEnemyController::BeginPlay()
{
	Super::BeginPlay();
	//绑定感知组件通知
	if (PerceptionComponent)
	{
		PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this,&ThisClass::OnTargetPerceptionUpdate);
	}
	
	
}

void AEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	//启动行为树
	if (AEnemyCharacter* EnemyChracter = Cast<AEnemyCharacter>(InPawn))
	{
		RunBehaviorTree(EnemyChracter->GetBehaviorTree());
		//更新行为树中 黑板数据
		if (GetBlackboardComponent())
		{
			GetBlackboardComponent()->SetValueAsVector(TEXT("BirthPosition"),InPawn->GetActorLocation());
			GetBlackboardComponent()->SetValueAsVector(TEXT("NavPosition"),InPawn->GetActorLocation()+InPawn->GetActorForwardVector()*1000);
		}
	}
}

ETeamAttitude::Type AEnemyController::GetTeamAttitudeTowards(const AActor& Other) const
{
	if (IsValid(GetPawn())&&IsValid(&Other))
	{
		if (ALgCharacterBase* MyCharacter = Cast<ALgCharacterBase>(GetPawn()))
		{
			if (const ALgCharacterBase* OtherCharacter = Cast<ALgCharacterBase>(&Other))
			{
				if (MyCharacter->GetTeamType() == ETeamType::ETT_None || OtherCharacter->GetTeamType() == ETeamType::ETT_None)
				{
					return ETeamAttitude::Neutral;
				}
				return MyCharacter->GetTeamType() == OtherCharacter->GetTeamType() ? ETeamAttitude::Friendly : ETeamAttitude::Hostile;
			}
		}
	}
	
	return Super::GetTeamAttitudeTowards(Other);
	
}

void AEnemyController::OnTargetPerceptionUpdate(AActor* Actor, FAIStimulus Stimulus)
{
	//区分是那种感知
	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())//判断是否是听觉感知
	{
		//更新到黑板
		GetBlackboardComponent()->SetValueAsVector(TEXT("NoisePosition"),Stimulus.StimulusLocation);
	}
	else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
	{
		//看到了和离开了视觉范围
		if (Stimulus.WasSuccessfullySensed())//如果真，则表明看到目标
		{
			//更新目标到黑板中
			const AActor* OldTarget = Cast<AActor>(GetBlackboardComponent()->GetValueAsObject(TEXT("Target")));
			if (IsValid(OldTarget))
			{
				//比较距离
				const FVector StandLocation = GetPawn()->GetActorLocation();
				if ((OldTarget->GetActorLocation()-StandLocation).Length()<(Actor->GetActorLocation()-StandLocation).Length())
				{
					return;
				}
			}
			GetBlackboardComponent()->SetValueAsObject(TEXT("Target"),Actor);
			
		}
		else//目标消失
		{
			
		}
	}
	
}

// Called every frame
void AEnemyController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

