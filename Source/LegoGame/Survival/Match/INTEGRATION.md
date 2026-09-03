# Survival Match production integration

`ASurvivalGameMode` uses only the public Survival Contracts in production. It
discovers exactly one authority-world `ISurvivalWorldRuntimeInterface` Actor and
exactly one `ISurvivalRuntimeSpawnInterface` Actor. Zero or multiple providers
are integration errors: layout, countdown, and directors do not start, and
Match never falls back to a Stub.

The layout state is polled from `FSurvivalWorldRuntimeSnapshot`. `NotRequested`
issues one request, `Generating` remains waiting, `Succeeded` permits team
countdown, and `Failed` prevents the match from starting. The GameState copies
only the UI-facing readiness and materialized room count; it never copies
World-private plans or Actors. Match advances World only when the requested
phase is strictly newer than the provider snapshot, then commits phase budgets
only after the snapshot confirms that phase.

Resource and enemy directors choose enabled valid public anchors using the
server `RandomStream`, seeded from `USurvivalModeConfig::RandomSeed`. Resource
tags are Match-local configuration and never contain Core ItemIds. Enemy
difficulty is `max(1.0, HungerDrainMultiplier, ThirstDrainMultiplier)`. Budgets
change only after a successful Contract result. Enemies remain actual Actor
references in `AliveEnemyActors`; the existing death listener removes only
tracked enemy Actors.

`DepositTeamRespawnEnergy` is the production ingress for team respawn energy.
It validates the source and team, atomically consumes `Item.RespawnEnergy`
through `ISurvivalInventoryInterface`, then credits the team pool. The older
`AddTeamRespawnEnergy` and `NotifyDirectorEnemyDefeated` are deprecated,
development-Stub-only helpers and do nothing in the production path.

`bUseDevelopmentIntegrationStubs` defaults to false and is forced false in
Shipping. Stubs remain under `Integration/Stubs` for explicit development tests
only; they cannot mix with production providers.

## Map handoff

Before a production Survival map can start, place one configured World runtime
generator and one configured `ASurvivalCoreRuntimeProvider`. Configure formal
resource ItemTags in the Props tables, configure the provider's `BP_Enemy`
default/archetypes, and provide enabled `Anchor.Resource`, `Anchor.Enemy`, and
team `Anchor.PlayerStart` anchors. Missing team starts leave players queued or
spectating; Match never substitutes world origin or generic starts.
