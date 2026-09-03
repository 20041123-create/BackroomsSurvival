#include "SurvivalWorkbenchRuntimeSpawner.h"

#include "EngineUtils.h"
#include "LegoGame/Survival/Contracts/SurvivalGameplayTags.h"
#include "LegoGame/Survival/Contracts/SurvivalInterfaces.h"
#include "LegoGame/Survival/SurvivalWorkbenchActor.h"
#include "TimerManager.h"

ASurvivalWorkbenchRuntimeSpawner::ASurvivalWorkbenchRuntimeSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	SetReplicateMovement(false);
}

void ASurvivalWorkbenchRuntimeSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	RefreshWorkbenches();
	GetWorldTimerManager().SetTimer(RefreshTimerHandle, this, &ThisClass::RefreshWorkbenches,
		FMath::Max(RefreshIntervalSeconds, 0.05f), true);
}

void ASurvivalWorkbenchRuntimeSpawner::RefreshWorkbenches()
{
	if (!HasAuthority() || !ResolveWorldRuntimeProvider())
	{
		return;
	}

	const FSurvivalWorldRuntimeSnapshot Snapshot = ISurvivalWorldRuntimeInterface::Execute_GetWorldRuntimeSnapshot(WorldRuntimeProvider.Get());
	if (Snapshot.LayoutStatus == ESurvivalWorldLayoutStatus::Failed)
	{
		if (!bLoggedLayoutFailure)
		{
			UE_LOG(LogTemp, Error, TEXT("Survival workbench runtime spawner: World layout failed; workbenches will not be materialized. Reason: %s"),
				*Snapshot.FailureReason);
			bLoggedLayoutFailure = true;
		}
		return;
	}
	if (Snapshot.LayoutStatus != ESurvivalWorldLayoutStatus::Succeeded || !Snapshot.bSucceeded)
	{
		return;
	}

	TArray<FSurvivalAnchorView> Anchors;
	ISurvivalWorldRuntimeInterface::Execute_GetAnchorsByTag(WorldRuntimeProvider.Get(), LG::SurvivalTags::Anchor_Workbench, Anchors);
	if (!bHasObservedSucceededLayout || HasRuntimeSignatureChanged(Snapshot) || HasMissingWorkbenchForEnabledAnchor(Anchors))
	{
		SynchronizeWorkbenches(Snapshot, Anchors);
	}
}

bool ASurvivalWorkbenchRuntimeSpawner::ResolveWorldRuntimeProvider()
{
	AActor* FoundProvider = nullptr;
	int32 ProviderCount = 0;
	for (TActorIterator<AActor> Iterator(GetWorld()); Iterator; ++Iterator)
	{
		AActor* Candidate = *Iterator;
		if (IsValid(Candidate) && Candidate->GetClass()->ImplementsInterface(USurvivalWorldRuntimeInterface::StaticClass()))
		{
			FoundProvider = Candidate;
			++ProviderCount;
		}
	}

	if (ProviderCount != 1)
	{
		WorldRuntimeProvider.Reset();
		if (!bLoggedProviderError)
		{
			UE_LOG(LogTemp, Error, TEXT("Survival workbench runtime spawner requires exactly one World runtime provider; found %d."), ProviderCount);
			bLoggedProviderError = true;
		}
		return false;
	}

	WorldRuntimeProvider = FoundProvider;
	return true;
}

bool ASurvivalWorkbenchRuntimeSpawner::HasRuntimeSignatureChanged(const FSurvivalWorldRuntimeSnapshot& Snapshot) const
{
	return Snapshot.LayoutHash != LastLayoutHash
		|| Snapshot.CurrentUnlockedPhaseIndex != LastUnlockedPhaseIndex
		|| Snapshot.MaterializedRoomCount != LastMaterializedRoomCount;
}

bool ASurvivalWorkbenchRuntimeSpawner::HasMissingWorkbenchForEnabledAnchor(const TArray<FSurvivalAnchorView>& Anchors) const
{
	for (const FSurvivalAnchorView& Anchor : Anchors)
	{
		if (IsUsableWorkbenchAnchor(Anchor)
			&& (!SpawnedWorkbenchesByRoom.Contains(Anchor.RoomHandle.Value)
				|| !SpawnedWorkbenchesByRoom.FindRef(Anchor.RoomHandle.Value).IsValid()))
		{
			return true;
		}
	}
	return false;
}

void ASurvivalWorkbenchRuntimeSpawner::SynchronizeWorkbenches(const FSurvivalWorldRuntimeSnapshot& Snapshot,
	const TArray<FSurvivalAnchorView>& Anchors)
{
	if (!WorkbenchActorClass)
	{
		if (!bLoggedClassError)
		{
			UE_LOG(LogTemp, Error, TEXT("Survival workbench runtime spawner has no WorkbenchActorClass. Configure the production BP_SurvivalWorkbench Blueprint on the map actor."));
			bLoggedClassError = true;
		}
		return;
	}

	TSet<int32> DuplicateRoomHandles;
	TSet<int32> SeenRoomHandles;
	for (const FSurvivalAnchorView& Anchor : Anchors)
	{
		if (IsUsableWorkbenchAnchor(Anchor) && SeenRoomHandles.Contains(Anchor.RoomHandle.Value))
		{
			DuplicateRoomHandles.Add(Anchor.RoomHandle.Value);
		}
		SeenRoomHandles.Add(Anchor.RoomHandle.Value);
	}

	for (const FSurvivalAnchorView& Anchor : Anchors)
	{
		if (!IsUsableWorkbenchAnchor(Anchor))
		{
			continue;
		}
		if (DuplicateRoomHandles.Contains(Anchor.RoomHandle.Value))
		{
			if (!LoggedDuplicateRoomHandles.Contains(Anchor.RoomHandle.Value))
			{
				UE_LOG(LogTemp, Error, TEXT("Survival workbench runtime spawner ignored duplicate enabled workbench anchors for RoomHandle %d."), Anchor.RoomHandle.Value);
				LoggedDuplicateRoomHandles.Add(Anchor.RoomHandle.Value);
			}
			continue;
		}
		if (SpawnedWorkbenchesByRoom.FindRef(Anchor.RoomHandle.Value).IsValid())
		{
			continue;
		}
		TrySpawnWorkbench(Anchor);
	}

	RememberRuntimeSignature(Snapshot);
}

bool ASurvivalWorkbenchRuntimeSpawner::TrySpawnWorkbench(const FSurvivalAnchorView& Anchor)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ASurvivalWorkbenchActor* Workbench = World->SpawnActor<ASurvivalWorkbenchActor>(WorkbenchActorClass, Anchor.Transform, SpawnParameters);
	if (!IsValid(Workbench))
	{
		UE_LOG(LogTemp, Error, TEXT("Survival workbench runtime spawner failed to spawn a workbench for RoomHandle %d."), Anchor.RoomHandle.Value);
		return false;
	}

	if (!Workbench->GetIsReplicated() || !Workbench->GetActorTransform().Equals(Anchor.Transform, KINDA_SMALL_NUMBER))
	{
		UE_LOG(LogTemp, Error, TEXT("Survival workbench runtime spawner rejected an invalid workbench spawn for RoomHandle %d."), Anchor.RoomHandle.Value);
		Workbench->Destroy();
		return false;
	}

	SpawnedWorkbenchesByRoom.Add(Anchor.RoomHandle.Value, Workbench);
	return true;
}

bool ASurvivalWorkbenchRuntimeSpawner::IsUsableWorkbenchAnchor(const FSurvivalAnchorView& Anchor) const
{
	return Anchor.AnchorTag.MatchesTagExact(LG::SurvivalTags::Anchor_Workbench)
		&& Anchor.bEnabled
		&& Anchor.RoomHandle.IsValid()
		&& !Anchor.Transform.ContainsNaN();
}

void ASurvivalWorkbenchRuntimeSpawner::RememberRuntimeSignature(const FSurvivalWorldRuntimeSnapshot& Snapshot)
{
	bHasObservedSucceededLayout = true;
	LastLayoutHash = Snapshot.LayoutHash;
	LastUnlockedPhaseIndex = Snapshot.CurrentUnlockedPhaseIndex;
	LastMaterializedRoomCount = Snapshot.MaterializedRoomCount;
}
