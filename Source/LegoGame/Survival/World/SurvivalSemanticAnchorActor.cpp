#include "SurvivalSemanticAnchorActor.h"

#include "Net/UnrealNetwork.h"

ASurvivalSemanticAnchorActor::ASurvivalSemanticAnchorActor()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	// Runtime navigation projection can relocate an anchor after materialization.
	// Replicate that final transform so public anchor views agree on every peer.
	SetReplicateMovement(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;
}

void ASurvivalSemanticAnchorActor::InitializeAnchor(const FGameplayTag InAnchorTag, const FRoomHandle InRoomHandle, const ETeamType InTeamType)
{
	if (!HasAuthority())
	{
		return;
	}
	AnchorTag = InAnchorTag;
	OwningRoomHandle = InRoomHandle;
	TeamType = InTeamType;
	ApplyActorTag();
	ForceNetUpdate();
}

void ASurvivalSemanticAnchorActor::SetAnchorEnabled(const bool bEnabled)
{
	if (!HasAuthority() || bAnchorEnabled == bEnabled)
	{
		return;
	}
	bAnchorEnabled = bEnabled;
	ForceNetUpdate();
}

void ASurvivalSemanticAnchorActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASurvivalSemanticAnchorActor, AnchorTag);
	DOREPLIFETIME(ASurvivalSemanticAnchorActor, OwningRoomHandle);
	DOREPLIFETIME(ASurvivalSemanticAnchorActor, TeamType);
	DOREPLIFETIME(ASurvivalSemanticAnchorActor, bAnchorEnabled);
}

void ASurvivalSemanticAnchorActor::OnRep_AnchorDefinition()
{
	ApplyActorTag();
}

void ASurvivalSemanticAnchorActor::ApplyActorTag()
{
	Tags.Reset();
	if (AnchorTag.IsValid())
	{
		Tags.Add(AnchorTag.GetTagName());
	}
}
