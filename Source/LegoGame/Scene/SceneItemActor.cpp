#include "SceneItemActor.h"

#include "Components/BillboardComponent.h"
#include "LegoGame/Character/LgCharacterBase.h"
#include "LegoGame/Components/PackageComponent.h"
#include "LegoGame/Subsystem/PropsSubsystem.h"
#include "Net/UnrealNetwork.h"

ASceneItemActor::ASceneItemActor()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("BillBoardComponent"));
	BillboardComponent->SetupAttachment(RootComponent);
	bReplicates = true;
	ID = INDEX_NONE;
}

void ASceneItemActor::BeginPlay()
{
	Super::BeginPlay();
	InitMesh();
}

void ASceneItemActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ASceneItemActor, ID, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ASceneItemActor, ItemStack, COND_InitialOnly);
}

void ASceneItemActor::InitMesh()
{
	const int32 ItemId = GetID();
	if (ItemId < 0 || StaticMeshComponent || SkeletalMeshComponent)
	{
		return;
	}

	UPropsSubsystem* PropsSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UPropsSubsystem>();
	const FPropsBase* Props = PropsSubsystem ? PropsSubsystem->GetPropsById(ItemId) : nullptr;
	if (!Props)
	{
		return;
	}

	if (Props->Type == EPropsType::EPT_Skin)
	{
		const FSkinHeader* SkinHeader = static_cast<const FSkinHeader*>(Props);
		if (SkinHeader->StaticMesh)
		{
			StaticMeshComponent = NewObject<UStaticMeshComponent>(this);
			StaticMeshComponent->RegisterComponentWithWorld(GetWorld());
			StaticMeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			StaticMeshComponent->SetCollisionProfileName(TEXT("SceneItemProfile"));
			StaticMeshComponent->SetStaticMesh(SkinHeader->StaticMesh);
		}
		else if (SkinHeader->SkeletalMesh)
		{
			SkeletalMeshComponent = NewObject<USkeletalMeshComponent>(this);
			SkeletalMeshComponent->RegisterComponentWithWorld(GetWorld());
			SkeletalMeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			SkeletalMeshComponent->SetCollisionProfileName(TEXT("SceneItemProfile"));
			SkeletalMeshComponent->SetSkeletalMesh(SkinHeader->SkeletalMesh);
			SkeletalMeshComponent->SetGenerateOverlapEvents(true);
		}
	}
	else if (Props->Type == EPropsType::EPT_Weapon)
	{
		const FWeaponBaseHeader* WeaponHeader = static_cast<const FWeaponBaseHeader*>(Props);
		SkeletalMeshComponent = NewObject<USkeletalMeshComponent>(this);
		SkeletalMeshComponent->RegisterComponentWithWorld(GetWorld());
		SkeletalMeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		SkeletalMeshComponent->SetCollisionProfileName(TEXT("SceneItemProfile"));
		SkeletalMeshComponent->SetSkeletalMesh(WeaponHeader->SkeletalMesh);
		SkeletalMeshComponent->SetGenerateOverlapEvents(true);
	}
}

void ASceneItemActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASceneItemActor::SetID(int32 NewID)
{
	ID = NewID;
	ItemStack.ItemId = NewID;
	ItemStack.Quantity = 1;
	ItemStack.SlotId = INDEX_NONE;
}

void ASceneItemActor::SetItemStack(const FItemStack& NewItemStack)
{
	ItemStack = NewItemStack;
	ItemStack.SlotId = INDEX_NONE;
	ID = ItemStack.ItemId;
}

void ASceneItemActor::OnRep_ItemStack()
{
	InitMesh();
}

bool ASceneItemActor::CanInteract_Implementation(APawn* InstigatorPawn) const
{
	return ItemStack.IsValid() && IsValid(InstigatorPawn)
		&& FVector::DistSquared(InstigatorPawn->GetActorLocation(), GetActorLocation()) <= FMath::Square(250.0f);
}

void ASceneItemActor::Interact_Implementation(APawn* InstigatorPawn)
{
	if (!HasAuthority() || !CanInteract_Implementation(InstigatorPawn))
	{
		return;
	}

	if (ALgCharacterBase* Character = Cast<ALgCharacterBase>(InstigatorPawn))
	{
		if (UPackageComponent* Package = Character->GetPackageComponent())
		{
			Package->PickItemFromNear(this);
		}
	}
	}
