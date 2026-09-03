"""Validate Survival World-owned room templates and map generator references."""

import unreal


ROOT = "/Game/LegoGame/Survival"
ROOM_ROOT = ROOT + "/Data/Rooms"
MAP_PATH = ROOT + "/Maps/L_SurvivalWorld"
MATERIAL_ROOT = ROOT + "/Materials"

EXPECTED_ROOMS = {
    "DA_Room_Base": ("Survival.Base", (2, 2), 0.0),
    "DA_Room_NormalStraight": ("Survival.NormalStraight", (1, 1), 1.0),
    "DA_Room_NormalCorner": ("Survival.NormalCorner", (1, 1), 1.0),
    "DA_Room_NormalHall": ("Survival.NormalHall", (2, 1), 0.8),
    "DA_Room_NormalPillarHall": ("Survival.NormalPillarHall", (2, 2), 0.45),
    "DA_Room_NormalPartitioned": ("Survival.NormalPartitioned", (2, 2), 0.4),
    "DA_Room_DarkCorridor": ("Survival.DarkCorridor", (1, 2), 0.3),
    "DA_Room_Monster": ("Survival.Monster", (2, 2), 0.7),
    "DA_Room_HighResource": ("Survival.HighResource", (1, 2), 0.35),
}

EXPECTED_MATERIALS = (
    "M_Survival_Backrooms_Carpet",
    "M_Survival_Backrooms_Linoleum",
    "M_Survival_Backrooms_CeilingPanels",
    "M_Survival_Backrooms_CeilingFrame",
    "M_Survival_Backrooms_CeilingVent",
    "M_Survival_Backrooms_Lamp",
    "M_Survival_Backrooms_Wallpaper",
    "M_Survival_Backrooms_Plaster",
    "M_Survival_Backrooms_PlasterWhite",
    "M_Survival_Backrooms_WallTrim",
    "M_Survival_Backrooms_WallTrimDark",
)


def require_asset(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError("Missing asset: {}".format(path))
    return asset


def verify_room(name, expected):
    asset = require_asset("{}/{}".format(ROOM_ROOT, name))
    template_id = str(asset.get_editor_property("template_id"))
    footprint = asset.get_editor_property("footprint")
    weight = float(asset.get_editor_property("generation_weight"))
    actual = (template_id, (footprint.x, footprint.y), weight)
    if actual[0:2] != expected[0:2] or abs(actual[2] - expected[2]) > 0.0001:
        raise RuntimeError("{} mismatch: expected {}, got {}".format(name, expected, actual))
    return asset


def verify_map(expected_assets):
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level_subsystem.load_level(MAP_PATH)
    generators = [
        actor
        for actor in actor_subsystem.get_all_level_actors()
        if actor.get_class().get_name() == "SurvivalWorldGenerator"
    ]
    if len(generators) != 1:
        raise RuntimeError("Expected one SurvivalWorldGenerator, got {}".format(len(generators)))
    obsolete_lights = {
        actor.get_actor_label()
        for actor in actor_subsystem.get_all_level_actors()
        if actor.get_actor_label() in {"SurvivalDirectionalLight", "SurvivalSkyLight"}
    }
    if obsolete_lights:
        raise RuntimeError(
            "Obsolete graybox global lights remain: {}".format(sorted(obsolete_lights))
        )
    post_process_volumes = [
        actor
        for actor in actor_subsystem.get_all_level_actors()
        if actor.get_class().get_name() == "PostProcessVolume"
    ]
    if len(post_process_volumes) != 1:
        raise RuntimeError(
            "Expected one Survival PostProcessVolume, got {}".format(
                len(post_process_volumes)
            )
        )
    post_process = post_process_volumes[0]
    post_process_settings = post_process.get_editor_property("settings")
    if not post_process.get_editor_property("unbound"):
        raise RuntimeError("Survival PostProcessVolume is not unbound")
    if not post_process_settings.get_editor_property("override_bloom_intensity"):
        raise RuntimeError("Survival PostProcessVolume does not override Bloom Intensity")
    if abs(float(post_process_settings.get_editor_property("bloom_intensity")) - 3.0) > 0.001:
        raise RuntimeError("Survival PostProcessVolume Bloom Intensity is not 3")

    generator = generators[0]
    actual_paths = {
        template.get_path_name().split(".")[0]
        for template in generator.get_editor_property("room_templates")
        if template
    }
    expected_paths = {
        asset.get_path_name().split(".")[0]
        for asset in expected_assets
    }
    if actual_paths != expected_paths:
        raise RuntimeError(
            "Generator room templates mismatch: expected {}, got {}".format(
                sorted(expected_paths), sorted(actual_paths)
            )
        )

    start_room = generator.get_editor_property("start_room_template")
    if not start_room or str(start_room.get_editor_property("template_id")) != "Survival.Base":
        raise RuntimeError("Generator start room is not Survival.Base")
    if abs(float(generator.get_editor_property("grid_cell_size")) - 1800.0) > 0.001:
        raise RuntimeError("Generator grid cell size is not 1800")


def verify_materials():
    for name in EXPECTED_MATERIALS:
        material = require_asset("{}/{}".format(MATERIAL_ROOT, name))
        if not material.get_editor_property("used_with_instanced_static_meshes"):
            raise RuntimeError("{} is not enabled for instanced static meshes".format(name))


def main():
    assets = [
        verify_room(name, expected)
        for name, expected in EXPECTED_ROOMS.items()
    ]
    verify_map(assets)
    verify_materials()
    unreal.log(
        "SURVIVAL_ASSET_VERIFY PASS: {} rooms, {} materials, and map references".format(
            len(assets), len(EXPECTED_MATERIALS)
        )
    )


main()
