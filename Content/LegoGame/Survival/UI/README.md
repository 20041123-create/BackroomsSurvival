# Survival UI ownership

`ASurvivalHUD` creates `USurvivalHUDWidget` as an asset-free native fallback so
listen-server and dedicated-server tests do not depend on editor-created assets.

Production artists should create the following Widget Blueprints in this folder
or replace their classes through the Survival HUD defaults:

- `WBP_SurvivalHUD`
- `WBP_SurvivalPhaseBanner`
- `WBP_SurvivalTeamResources`
- `WBP_SurvivalRespawn`
- `WBP_SurvivalResults`

`WBP_SurvivalHUD`, `WBP_SurvivalPhaseBanner`, `WBP_SurvivalTeamResources`, and
`WBP_SurvivalResults` currently use `USurvivalHUDWidget`. The dedicated
`WBP_SurvivalRespawn` uses `USurvivalRespawnWidget` and must retain the
`TextBlock_Num` and `TextBlock_Show` bindings.

The widget binds only to `ASurvivalGameState` and `ASurvivalPlayerState`; it
must not query Core inventory, World room actors, or private implementation
types.
