# Survival World integration request

World now implements the integration-owned `ISurvivalWorldRuntimeInterface`
without adding dependencies from Contracts back into World.

## Random Room plugin bridge

`FSurvivalLayoutPlanner` is now an asset-compatible bridge to
`Plugins/RandomRoomGeneration`. It copies the existing `URoomTemplateData` and
`USurvivalModeConfig` inputs into project-neutral definitions, invokes the plugin
planner, then copies the result back into the existing World-only types. Existing
Survival assets, map references, and replication properties therefore remain
unchanged. The plugin's generic runtime Actors are available to other projects;
Survival continues to own its semantic anchors and compatibility actors pending
an integration-owned Contracts API.

## Runtime provider

The existing map-owned `ASurvivalWorldGenerator` is the production runtime
provider and directly implements `ISurvivalWorldRuntimeInterface`. Its lifetime
is therefore the lifetime of the generator already placed in the Survival
world; no adapter Actor or second source of World state is created. Match should
discover exactly one Actor implementing `USurvivalWorldRuntimeInterface` on the
authoritative world and treat zero or multiple providers as an integration
error.

Both explicit generation requests and `bGenerateOnBeginPlay` enter the same
one-shot state machine. `LayoutStatus` and `GenerationFailureReason` replicate
alongside the existing seed, hash, phase, room-count, and success fields.
State-changing calls also perform an explicit `HasAuthority()` check.

Anchor queries iterate only `ASurvivalSemanticAnchorActor` instances whose
replicated Actor owner is this generator, convert them to public views, and
sort them by room handle, team, location, and rotation. Team spawn selection
uses the first enabled, exact-team `Anchor.PlayerStart` in that stable order and
does not fall back to the origin or a generic spawn.

Base-room respawn semantics now use separate Police and Bandit
`Anchor.RespawnBase` anchors. `Anchor.TeamTerminal` remains a distinct tag and
is no longer spawned as a respawn fallback.

The editor-only `ASurvivalWorldPhaseTestDriver` remains a manual PIE smoke
driver, but now advances phases through the production Contracts interface.

## Match migration

1. Discover the single authoritative runtime-interface provider.
2. Read the snapshot before requesting generation so an already completed
   `bGenerateOnBeginPlay` layout is accepted as the current result.
3. If status is `NotRequested`, submit the current `USurvivalModeConfig`; poll
   until `Succeeded` or `Failed`.
4. Submit strictly increasing configured phase indices through
   `RequestAdvanceToPhase`.
5. Resolve team spawns through `GetTeamPlayerStartTransform`; on false, keep the
   player waiting/spectating and retry or report an integration failure.

Core and Match must continue to consume only the eventual Contracts interface
and the existing anchor tags; they must not include these World headers.
