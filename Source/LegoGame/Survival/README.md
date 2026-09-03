# Survival worktree contracts

This directory is the integration boundary for the Survival mode worktrees.

## Ownership

- `codex/survival-core` owns inventory, items, interaction, health, needs,
  damage rules, crafting, and death loot. It may modify the existing
  `PackageComponent`, `PropsSubsystem`, `SceneItemActor`, `LgCharacterBase`,
  and `WeaponBase` implementations.
- `codex/survival-world` owns room templates, generation, connectors, gates,
  anchors, navigation integration, and all Survival room/map assets.
- `codex/survival-match` owns the Survival GameMode/GameState, phase and
  encounter directors, respawn and victory rules, and Survival HUD/UMG assets.

## Integration rules

- Feature branches depend on declarations in `Contracts`; they do not include
  another feature branch's private implementation headers.
- Binary Unreal assets have one owning branch and are never edited in parallel.
- Changes to shared contracts land on `codex/survival-integration` first, then
  feature branches update from that branch.
- Feature branches merge only into `codex/survival-integration`.
