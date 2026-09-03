# Survival runtime contracts

Survival Contracts are the only implementation boundary between Match and the
Core and World runtime providers. Match discovers providers on the
authoritative world and must never include Core inventory, item, enemy, World
generator, room, gate, or semantic-anchor implementation headers.

## Match-to-World runtime

`ISurvivalWorldRuntimeInterface` owns layout planning, room spawning, gates,
and anchors.

- Match first reads `GetWorldRuntimeSnapshot`.
- It calls `RequestGenerateInitialLayout` only while the status is
  `NotRequested`; the request is one-shot for the provider lifetime.
- `Generating` is non-terminal. Match polls the snapshot from its server update
  loop until `Succeeded` or `Failed`.
- `bSucceeded` is true only for `Succeeded`. A failed operation supplies a
  diagnostic `FailureReason`; the snapshot retains the last confirmed phase and
  materialized-room count.
- `RequestAdvanceToPhase` is server-authoritative and strictly monotonic.
  Requests for the current or a lower phase are rejected. World decides how to
  materialize rooms and update gates for an accepted target phase.

`GetAnchorsByTag` returns public `FSurvivalAnchorView` values, never anchor
Actors. It includes disabled anchors so Match can make an explicit decision.
`GetTeamPlayerStartTransform` filters to an enabled `Anchor.PlayerStart` for
the requested Police or Bandit team. A false return means no spawn transform
was found; callers must not substitute the world origin or generic player
start. `Anchor.RespawnBase` is the dedicated respawn-base semantic tag;
`Anchor.TeamTerminal` is not a respawn fallback.

## Tagged inventory consumption

`ISurvivalInventoryInterface` represents one concrete inventory owner. Match
must call `GetItemQuantityByTag` and `TryConsumeItemsByTag` on that owner rather
than enumerating slots and issuing individual removals.

- A valid query tag matches an item view when
  `FSurvivalItemView.ItemTags.HasTag(ItemTag)` is true. This is standard
  GameplayTag hierarchy matching: an item child tag matches a parent query, but
  a parent item tag does not match a more specific child query.
- Invalid tags return zero. Only valid stacks with positive quantities count.
- `TryConsumeItemsByTag` is server-authoritative. Implementations must reject
  non-authority calls, invalid tags, and non-positive quantities in C++.
- Consumption is atomic: the implementation verifies the aggregate total
  before mutation, resolves matching stacks in ascending SlotId order, and
  commits all removals only after the complete request can succeed. A failure
  changes no stack.
- A successful operation emits one inventory change notification and one
  replication update. `Item.RespawnEnergy` is the formal respawn-energy tag.

## Match-to-Core runtime spawning

`ISurvivalRuntimeSpawnInterface` creates exactly one real, initialized,
replicated Actor per successful server-authoritative request.

- `TrySpawnResource` validates a non-empty ItemTag and positive Quantity; Core
  resolves the tag to a private item definition and creates the real pickup.
- `TrySpawnEnemy` validates a strictly positive difficulty multiplier. An empty
  EnemyArchetypeTag requests the Core-configured default enemy; a valid tag
  requests that archetype. Core applies the multiplier to the real enemy
  configuration.
- `FRoomHandle` is public context only. Requests never expose World-private
  room types, DataTable rows, ItemIds, SceneItemActor, EnemyCharacter, or AI
  controller classes.
- `FSurvivalRuntimeSpawnResult` defaults to failure. Success requires
  `bSucceeded`, result code `Succeeded`, and a valid Actor. Failures return no
  Actor and use the result code for control flow; `FailureReason` is diagnostic.
- Match owns budgets, frequencies, MaxAliveEnemies, phase transitions, room
  unlocking, and victory. It deducts budget or increments alive-enemy counts
  only after a successful result. Enemy death remains on the existing Survival
  Death Listener path.

## Weapon ammunition presentation

`ISurvivalWeaponStateInterface` exposes one read-only
`FSurvivalWeaponAmmoSnapshot` for presentation. Match and HUD code must not
include the Core character, weapon, or package implementation to obtain clip
state. The equipped character supplies loaded ammunition, clip capacity, and
reserve ammunition from its ordinary owner-only replicated state. An
unequipped or unavailable weapon returns the safe default snapshot.

## Provider discovery and implementation requirements

There must be exactly one production Actor implementing
`USurvivalRuntimeSpawnInterface` in the authoritative world. Match calls it
only through the interface. Zero or multiple providers are explicit integration
failures; Match must not fall back to a count-only Stub.

All state-changing functions must reject non-authority callers even when
invoked from C++. Queries and providers must be safe on listen and dedicated
servers and must not require a local PlayerController, HUD, Viewport, or
client-only object. Successful Actors follow their ordinary replication and
lifecycle behavior.
