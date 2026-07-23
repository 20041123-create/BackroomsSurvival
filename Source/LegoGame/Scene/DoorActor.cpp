#include "DoorActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/GameStateBase.h"
#include "LegoGame/Character/LgCharacterBase.h"
#include "Net/UnrealNetwork.h"

ADoorActor::ADoorActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	StaticMeshComponent->SetupAttachment(RootComponent);

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	BoxComponent->SetupAttachment(RootComponent);
	BoxComponent->SetCollisionProfileName(TEXT("Overlap"));
}

void ADoorActor::BeginPlay()
{
	Super::BeginPlay();

	float MinCurveTime = 0.0f;
	float MaxCurveTime = 0.0f;
	if (AnimeCurve)
	{
		AnimeCurve->GetTimeRange(MinCurveTime, MaxCurveTime);
	}
	CurrentTimeTotal = MinCurveTime;

	if (HasAuthority())
	{
		BoxComponent->OnComponentBeginOverlap.AddDynamic(
			this, &ThisClass::OnBoxComponentBeginOverlapEvent);
		BoxComponent->OnComponentEndOverlap.AddDynamic(
			this, &ThisClass::OnBoxComponentEndOverlapEvent);
		DoorState.StartCurveTime = MinCurveTime;
		DoorState.StateChangeServerTime = GetSynchronizedServerTime();
	}
	else
	{
		OnRep_DoorState();
	}
}

void ADoorActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADoorActor, DoorState);
}

void ADoorActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() && DoorState.bOpen)
	{
		for (auto Iterator = OverlappingCharacters.CreateIterator(); Iterator; ++Iterator)
		{
			if (!Iterator.Key().IsValid())
			{
				Iterator.RemoveCurrent();
			}
		}
		if (OverlappingCharacters.IsEmpty())
		{
			SetDoorOpen(false, DoorState.OpenDirection);
		}
	}

	UpdateDoorAnimation(DeltaTime);
}

void ADoorActor::UpdateDoorAnimation(float DeltaTime)
{
	if (!AnimeCurve || !bDoorAnimation)
	{
		return;
	}

	CurrentTimeTotal += DeltaTime * (DoorState.bOpen ? 1.0f : -1.0f);

	float MinCurveTime = 0.0f;
	float MaxCurveTime = 0.0f;
	AnimeCurve->GetTimeRange(MinCurveTime, MaxCurveTime);
	CurrentTimeTotal = FMath::Clamp(CurrentTimeTotal, MinCurveTime, MaxCurveTime);

	const float CurveValue = AnimeCurve->GetFloatValue(CurrentTimeTotal);
	StaticMeshComponent->SetRelativeRotation(
		FRotator(0.0f, CurveValue * 90.0f * DoorState.OpenDirection, 0.0f));

	if (CurrentTimeTotal <= MinCurveTime || CurrentTimeTotal >= MaxCurveTime)
	{
		bDoorAnimation = false;
	}
}

void ADoorActor::OnBoxComponentBeginOverlapEvent(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	ALgCharacterBase* Character = Cast<ALgCharacterBase>(OtherActor);
	if (!IsValid(Character))
	{
		return;
	}

	const bool bWasEmpty = OverlappingCharacters.IsEmpty();
	const TWeakObjectPtr<ALgCharacterBase> CharacterKey(Character);
	int32& OverlapCount = OverlappingCharacters.FindOrAdd(CharacterKey);
	++OverlapCount;
	if (bWasEmpty)
	{
		const FVector StandDirection = (Character->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		const float NewDirection = FVector::DotProduct(StandDirection, GetActorRightVector()) > 0.0f
			? -1.0f
			: 1.0f;
		SetDoorOpen(true, NewDirection);
	}
}

void ADoorActor::OnBoxComponentEndOverlapEvent(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (!HasAuthority())
	{
		return;
	}

	if (ALgCharacterBase* Character = Cast<ALgCharacterBase>(OtherActor))
	{
		const TWeakObjectPtr<ALgCharacterBase> CharacterKey(Character);
		if (int32* OverlapCount = OverlappingCharacters.Find(CharacterKey))
		{
			--(*OverlapCount);
			if (*OverlapCount <= 0)
			{
				OverlappingCharacters.Remove(CharacterKey);
			}
		}
	}
	for (auto Iterator = OverlappingCharacters.CreateIterator(); Iterator; ++Iterator)
	{
		if (!Iterator.Key().IsValid())
		{
			Iterator.RemoveCurrent();
		}
	}
	if (OverlappingCharacters.IsEmpty())
	{
		SetDoorOpen(false, DoorState.OpenDirection);
	}
}

void ADoorActor::SetDoorOpen(bool bOpen, float NewOpenDirection)
{
	if (!HasAuthority() || DoorState.bOpen == bOpen)
	{
		return;
	}

	DoorState.bOpen = bOpen;
	DoorState.OpenDirection = NewOpenDirection;
	DoorState.StartCurveTime = CurrentTimeTotal;
	DoorState.StateChangeServerTime = GetSynchronizedServerTime();
	bDoorAnimation = true;
	ForceNetUpdate();
}

float ADoorActor::GetSynchronizedServerTime() const
{
	if (const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr)
	{
		return GameState->GetServerWorldTimeSeconds();
	}
	return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
}

void ADoorActor::OnRep_DoorState()
{
	if (!AnimeCurve)
	{
		return;
	}

	float MinCurveTime = 0.0f;
	float MaxCurveTime = 0.0f;
	AnimeCurve->GetTimeRange(MinCurveTime, MaxCurveTime);

	const float ElapsedTime = FMath::Max(
		0.0f,
		GetSynchronizedServerTime() - DoorState.StateChangeServerTime);
	CurrentTimeTotal = DoorState.StartCurveTime
		+ ElapsedTime * (DoorState.bOpen ? 1.0f : -1.0f);
	CurrentTimeTotal = FMath::Clamp(CurrentTimeTotal, MinCurveTime, MaxCurveTime);
	bDoorAnimation = true;
	UpdateDoorAnimation(0.0f);
}
