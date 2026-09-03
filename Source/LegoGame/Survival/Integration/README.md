# Survival Core Runtime Provider

`ASurvivalCoreRuntimeProvider` is the authority-world implementation of the
Match-to-Core `USurvivalRuntimeSpawnInterface`. A Survival map must contain
exactly one configured instance; Match discovers it through the interface and
must treat zero or multiple providers as integration errors.

## Resource setup

`ResourceActorClass` defaults to `ASceneItemActor`. Resource resolution uses
`UPropsSubsystem::GetSurvivalItemIdsByTag`: it verifies the requested
GameplayTag with `FSurvivalItemView.ItemTags.HasTag`, sorts all matching ItemIds
ascending, and chooses the first. This keeps selection reproducible and never
exports ItemIds into Contracts or Match. Configure formal Survival tags and
stack limits on the corresponding prop-table rows before asking Match to spawn
that resource. A missing tagged row returns `NoMatchingDefinition`.

## Enemy setup

Set `DefaultEnemyClass` to the production enemy Blueprint (normally
`BP_Enemy`) for untagged requests. Set `DefaultEnemyArchetypeTag` for result
diagnostics, and populate `EnemyArchetypeClasses` for tagged archetypes. The
Provider only resolves and spawns one actor; Match retains all spawn budgets,
frequency limits, phases, alive counts, and victory decisions.

The Provider performs a deferred spawn and applies the request multiplier
before `BeginPlay`. For the spawned enemy only, the formula is
`MaxHealth = DefaultMaxHealth * DifficultyMultiplier`, and initial `Health`
equals that MaxHealth. The multiplier is finite, positive, authority-only, and
accepted exactly once before Vitals initialization. Hunger, thirst, movement,
damage scale, AI possession, and the existing Survival death listener retain
their existing behavior.

Both resource and enemy calls reject non-authority callers in C++ and return a
consistent `FSurvivalRuntimeSpawnResult`. The default collision policy is
`AdjustIfPossibleButDontSpawnIfColliding`; a null deferred spawn under this
policy reports `SpawnBlocked`. Use a production placement transform that is
valid for the target map.

## Production map configuration

`L_SurvivalWorld` owns one `ASurvivalWorldGenerator` and one
`ASurvivalCoreRuntimeProvider`. The generator uses
`DA_SurvivalMode_Default` and does **not** generate on BeginPlay: Match owns
the one-shot runtime request. The Core provider uses `ASceneItemActor` and
`BP_Enemy`, with an invalid enemy archetype tag so untagged Match requests use
the configured default. The map also contains one `ALgPlayerStart` for each
Police and Bandit team; these are map-authoring diagnostics while production
spawning uses the generated `Anchor.PlayerStart` transforms.

`BP_SurvivalGameMode` references `DA_SurvivalMode_Default`, has both
development flags disabled, and retains the verified Food, Water, Ammo, and
Material resource tags. There is currently no semantically valid formal
`Item.Category.RespawnEnergy` prop row, so it is deliberately excluded. Core /
Content must provide a real pickup definition before the production respawn
energy loop can be accepted.

`LegoGame.Survival.Integration.MapConfiguration` loads the map package and
checks these placement and default settings, including the absence of
`ASurvivalWorldPhaseTestDriver`.

## Respawn terminals and interaction

`L_SurvivalWorld` has exactly one `ASurvivalTeamRespawnTerminal` for each team,
using the Generator prop mesh and placed near that team's generated respawn
base. `APlayerCharacter` maps `IA_Interact` through the `Interact` row in
`DT_KeyMapping` (default **E**), chooses a nearby Contract interactable in a
stable nearest-first order, and sends it through the existing authoritative
`UPackageComponent::RequestInteract` RPC. The server repeats interface and
distance checks; the terminal additionally verifies that the player's team
matches its authored team before calling `DepositTeamRespawnEnergy`.

The interaction, terminal, tag and atomic consumption path are production
code. The final credit/respawn success path remains content-blocked: no formal
prop-table row, pickup mesh and icon have been approved for
`Item.Category.RespawnEnergy`. The map intentionally contains no substitute
or placeholder item.
