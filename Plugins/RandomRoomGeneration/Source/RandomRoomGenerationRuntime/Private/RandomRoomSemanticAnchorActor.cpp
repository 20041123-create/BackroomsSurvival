#include "RandomRoomSemanticAnchorActor.h"

#include "Net/UnrealNetwork.h"

ARandomRoomSemanticAnchorActor::ARandomRoomSemanticAnchorActor()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);
}

void ARandomRoomSemanticAnchorActor::InitializeAnchor(const FGameplayTag InAnchorTag, const FRandomRoomHandle InRoomHandle, const FGameplayTagContainer& InContextTags)
{
	if (!HasAuthority()) { return; }
	AnchorTag = InAnchorTag;
	OwningRoomHandle = InRoomHandle;
	ContextTags = InContextTags;
	ApplyActorTag();
	ForceNetUpdate();
}

void ARandomRoomSemanticAnchorActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARandomRoomSemanticAnchorActor, AnchorTag);
	DOREPLIFETIME(ARandomRoomSemanticAnchorActor, OwningRoomHandle);
	DOREPLIFETIME(ARandomRoomSemanticAnchorActor, ContextTags);
}

void ARandomRoomSemanticAnchorActor::OnRep_AnchorDefinition() { ApplyActorTag(); }
void ARandomRoomSemanticAnchorActor::ApplyActorTag()
{
	Tags.Reset();
	if (AnchorTag.IsValid()) { Tags.Add(AnchorTag.GetTagName()); }
}
