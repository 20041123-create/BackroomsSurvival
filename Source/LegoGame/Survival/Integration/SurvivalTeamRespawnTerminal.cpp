#include "LegoGame/Survival/Integration/SurvivalTeamRespawnTerminal.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "LegoGame/Survival/Contracts/SurvivalGameplayTags.h"
#include "LegoGame/Survival/Match/SurvivalGameMode.h"
#include "LegoGame/Survival/Match/SurvivalPlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalRespawnTerminal, Log, All);

ASurvivalTeamRespawnTerminal::ASurvivalTeamRespawnTerminal()
{
	bReplicates = true;
	SetReplicateMovement(false);
	PrimaryActorTick.bCanEverTick = false;
	TerminalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TerminalMesh"));
	SetRootComponent(TerminalMesh);
	TerminalMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TerminalMesh->SetCollisionResponseToAllChannels(ECR_Overlap);

	InteractionBounds = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionBounds"));
	InteractionBounds->SetupAttachment(TerminalMesh);
	InteractionBounds->InitSphereRadius(InteractionDistance);
	InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBounds->SetCollisionResponseToAllChannels(ECR_Overlap);
	InteractionBounds->SetGenerateOverlapEvents(true);
}

void ASurvivalTeamRespawnTerminal::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (InteractionBounds)
	{
		InteractionBounds->SetSphereRadius(FMath::Max(1.0f, InteractionDistance));
	}
}

bool ASurvivalTeamRespawnTerminal::CanInteract_Implementation(APawn* InstigatorPawn) const
{
	return InstigatorPawn && TeamType != ETeamType::ETT_None
		&& FVector::DistSquared(InstigatorPawn->GetActorLocation(), GetActorLocation()) <= FMath::Square(InteractionDistance);
}

void ASurvivalTeamRespawnTerminal::Interact_Implementation(APawn* InstigatorPawn)
{
	TryDepositRespawnEnergy(InstigatorPawn);
}

bool ASurvivalTeamRespawnTerminal::TryDepositRespawnEnergy(APawn* InstigatorPawn)
{
	if (!HasAuthority())
	{
		UE_LOG(LogSurvivalRespawnTerminal, Warning,
			TEXT("Respawn energy deposit rejected by %s: request was not executed on authority."), *GetName());
		return false;
	}
	if (!InstigatorPawn || DepositQuantity <= 0)
	{
		UE_LOG(LogSurvivalRespawnTerminal, Warning,
			TEXT("Respawn energy deposit rejected by %s: invalid pawn or deposit quantity."), *GetName());
		return false;
	}
	if (!CanInteract_Implementation(InstigatorPawn))
	{
		UE_LOG(LogSurvivalRespawnTerminal, Warning,
			TEXT("Respawn energy deposit rejected by %s: %s is %.1f cm away (maximum %.1f cm)."),
			*GetName(), *GetNameSafe(InstigatorPawn),
			FVector::Distance(InstigatorPawn->GetActorLocation(), GetActorLocation()), InteractionDistance);
		return false;
	}

	const ASurvivalPlayerState* PlayerState = InstigatorPawn->GetPlayerState<ASurvivalPlayerState>();
	ASurvivalGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ASurvivalGameMode>() : nullptr;
	if (!PlayerState || !GameMode)
	{
		UE_LOG(LogSurvivalRespawnTerminal, Warning,
			TEXT("Respawn energy deposit rejected by %s: Survival PlayerState or GameMode is unavailable."), *GetName());
		return false;
	}
	if (PlayerState->GetTeamType() != TeamType)
	{
		UE_LOG(LogSurvivalRespawnTerminal, Warning,
			TEXT("Respawn energy deposit rejected by %s: player team %d does not match terminal team %d."),
			*GetName(), static_cast<uint8>(PlayerState->GetTeamType()), static_cast<uint8>(TeamType));
		return false;
	}

	const int32 AvailableQuantity = ISurvivalInventoryInterface::Execute_GetItemQuantityByTag(
		InstigatorPawn, LG::SurvivalTags::Item_RespawnEnergy);
	if (AvailableQuantity < DepositQuantity)
	{
		UE_LOG(LogSurvivalRespawnTerminal, Warning,
			TEXT("Respawn energy deposit rejected by %s: inventory has %d, requires %d."),
			*GetName(), AvailableQuantity, DepositQuantity);
		return false;
	}

	const bool bDeposited = GameMode->DepositTeamRespawnEnergy(InstigatorPawn, TeamType, DepositQuantity);
	if (bDeposited)
	{
		UE_LOG(LogSurvivalRespawnTerminal, Display,
			TEXT("Respawn energy deposit succeeded at %s for %s (team %d, quantity %d)."),
			*GetName(), *GetNameSafe(InstigatorPawn), static_cast<uint8>(TeamType), DepositQuantity);
	}
	else
	{
		UE_LOG(LogSurvivalRespawnTerminal, Warning,
			TEXT("Respawn energy deposit was rejected by the match at %s for %s (team %d, quantity %d)."),
			*GetName(), *GetNameSafe(InstigatorPawn), static_cast<uint8>(TeamType), DepositQuantity);
	}
	return bDeposited;
}
