# Random Room Generation

`RandomRoomGeneration` is a project-neutral UE runtime plugin for deterministic,
grid-based room layouts. It contains no `LegoGame` includes and can be enabled by
another project through its `.uproject` plugin list.

## Modules

- `RandomRoomGeneration`: template/config DataAssets, portable layout definitions,
  and `FRandomRoomLayoutPlanner`.
- `RandomRoomGenerationRuntime`: replicated graybox room, gate, semantic-anchor,
  and world-generator Actors.

The planner always sorts candidates before using `FRandomStream`; matching inputs
and a matching engine version therefore produce the same layout hash. It rejects
overlap, invalid connectors, disconnected plans, and returns no partial plan when
all attempts fail.

## C++ integration

Build `FRandomRoomGenerationRequest` from `URandomRoomGenerationConfig` and
`URandomRoomTemplateData`, call
`RandomRoomGeneration::FRandomRoomLayoutPlanner::Generate`, then invoke
`ARandomRoomWorldGenerator::InitializeLayout` on the server. The generator
replicates its summary and materialized Actors; clients must never generate their
own layout. Call `AdvanceToPhase` on the server for phase expansion.

`URandomRoomTemplateData::RoomActorClass` is deliberately project-owned. The
generic runtime provides a graybox actor, while projects may subclass it or spawn
their own presentation from the `On Room Materialized` Blueprint event.

## Semantic anchors

`ARandomRoomSemanticAnchorActor` uses one primary gameplay tag and an arbitrary
tag container for context. This keeps team, faction, resource, spawn, and revive
semantics in the consuming project rather than coupling the plugin to a game mode.

## Survival compatibility

Survival retains its existing DataAssets, map, class paths, and replicated actor
properties. `FSurvivalLayoutPlanner` is a compatibility adapter over the plugin
planner, so existing Survival templates continue to generate the same deterministic
layout without asset migration. Its project-specific anchors and room actor classes
remain in `Source/LegoGame/Survival/World` until shared Contracts explicitly
define a cross-mode runtime API.

## Validation

The plugin owns `RandomRoomGeneration.Layout.*` automation tests. The host project
also runs `LegoGame.Survival.World.Layout.*`, which exercises the compatibility
adapter with Survival's registered gameplay tags and phase data.
