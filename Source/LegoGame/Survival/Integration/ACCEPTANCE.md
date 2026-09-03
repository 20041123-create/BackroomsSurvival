# Survival runtime acceptance

## Automated coverage

Run `LegoGame.Survival.Integration.MapConfiguration` to verify the production
map owns exactly one World provider, one Core provider, one Workbench runtime
spawner configured with `BP_SurvivalWorkbench`, two team starts, two team
respawn terminals, no generic `PlayerStart`, the six approved resource tags
(Food, Water, Material, Ammo, RespawnEnergy and Weapon), and the `8/16/24/32`
phase room-unlock progression. Run `LegoGame.Survival.Match` for production
provider discovery, layout/phase authority boundaries, director Contract result
handling, direct-join team balancing, and the formal RespawnEnergy tag.

`ASurvivalWorkbenchRuntimeSpawner` is an authority-only map adapter. It polls
the public `ISurvivalWorldRuntimeInterface` snapshot only to observe lifecycle
changes: it does nothing while the layout is pending, materializes all enabled
`Anchor.Workbench` anchors when the layout first succeeds, and later materializes
only newly enabled anchors after a phase signature change. It never uses the
resource director or a periodic item spawn budget. The map must contain exactly
one spawner and configure its `WorkbenchActorClass` to
`/Game/LegoGame/Survival/BP_SurvivalWorkbench.BP_SurvivalWorkbench_C`; do not
manually place workbenches or use the native actor as a production fallback.

The lightweight Match test world intentionally has no server net authority;
its layout and phase tests verify that Contract commands are rejected there.
Actual layout materialization and multiplayer sequencing require PIE because
they need an authority world plus connected clients.

## Manual PIE checklist

1. Open `L_SurvivalWorld` with `BP_SurvivalGameMode`; verify one generator and
   one Core runtime provider exist, then launch Listen Server PIE with two
   clients. Verify the generated layout transitions from waiting to countdown
   and that the clients receive different balanced teams.
2. Repeat with Dedicated Server PIE plus two clients. Verify layout generation,
   phase room growth at 300/600/900 seconds, provider discovery, team-specific
   generated spawn anchors, replicated enemy/resource Actors, and client HUD
   phase/team data.
3. In both PIE modes, once initial layout generation reports success, verify
   every enabled `Anchor.Workbench` has one replicated
   `BP_SurvivalWorkbench` at the anchor transform. Advance the World phase and
   verify newly enabled workbench anchors materialize once, without duplicate
   workbenches or any client-side spawn. Press **E** at a workbench with
   Material `204 x2`; verify `Weapon.MachineGun01` produces Item `100` in the
   equipable package, then verify equip, drop and death-drop replication.
4. For each team, approach its Generator terminal and press **E**. A matching
   team may only deposit a valid `Item.Category.RespawnEnergy` stack; the other
   team, out-of-range callers and non-authority clients must be rejected.
5. Kill a player after a successful deposit and verify team-pool reserve,
   waiting state, delay, respawn at that team's generated anchor, replicated
   life state, and no world-origin fallback.

## Known acceptance block

Step 3's successful deposit and step 4's energy-funded respawn are **blocked**
until Content supplies an approved, formal RespawnEnergy prop definition,
pickup mesh and icon. This branch deliberately does not create an arbitrary
ItemId or placeholder asset, so it cannot claim full respawn-energy or final
multiplayer acceptance yet. Dedicated-server packaging must also be attempted
with an engine distribution that supports Server targets; the current installed
UE 5.4 distribution reports that Server targets are unsupported.
