"""Create or update the Survival World-owned DataAssets and PIE map.

Run with UnrealEditor-Cmd and the PythonScriptPlugin/EditorScriptingUtilities
enabled on the command line. Asset ownership remains inside Survival World.
"""

import unreal


ROOT = "/Game/LegoGame/Survival"
DATA_ROOT = ROOT + "/Data"
ROOM_DATA_ROOT = DATA_ROOT + "/Rooms"
MAP_PATH = ROOT + "/Maps/L_SurvivalWorld"


def require_class(path):
    result = unreal.load_class(None, path)
    if not result:
        raise RuntimeError("Could not load class: {}".format(path))
    return result


ROOM_TEMPLATE_CLASS = require_class("/Script/LegoGame.RoomTemplateData")
MODE_CONFIG_CLASS = require_class("/Script/LegoGame.SurvivalModeConfig")
GENERATOR_CLASS = require_class("/Script/LegoGame.SurvivalWorldGenerator")
TEST_DRIVER_CLASS = require_class("/Script/LegoGame.SurvivalWorldPhaseTestDriver")
ROOM_RUNTIME_CLASS = require_class("/Script/LegoGame.SurvivalRoomRuntimeActor")


def gameplay_tag(path):
    tag = unreal.GameplayTag()
    tag.import_text(path)
    return tag


TAG_NORMAL = gameplay_tag("Room.Type.Normal")
TAG_MONSTER = gameplay_tag("Room.Type.Monster")
TAG_HIGH_RESOURCE = gameplay_tag("Room.Type.HighResource")


def make_container(tag):
    container = unreal.GameplayTagContainer()
    container.import_text("(GameplayTags=({}))".format(tag.export_text()))
    return container


def connector(connector_id, x, y, direction):
    value = unreal.RoomConnectorDefinition()
    value.set_editor_property("connector_id", unreal.Name(connector_id))
    value.set_editor_property("cell", unreal.IntPoint(x, y))
    value.set_editor_property("direction", direction)
    return value


NORTH = unreal.RoomConnectorDirection.NORTH
EAST = unreal.RoomConnectorDirection.EAST
SOUTH = unreal.RoomConnectorDirection.SOUTH
WEST = unreal.RoomConnectorDirection.WEST


def get_or_create_data_asset(name, data_class, package_path):
    asset_path = package_path + "/" + name
    existing = unreal.EditorAssetLibrary.load_asset(asset_path)
    if existing:
        return existing
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", data_class)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, package_path, data_class, factory)
    if not asset:
        raise RuntimeError("Failed to create {}".format(asset_path))
    return asset


def configure_room(name, template_id, footprint, weight, room_type, connectors):
    asset = get_or_create_data_asset(name, ROOM_TEMPLATE_CLASS, ROOM_DATA_ROOT)
    asset.set_editor_property("template_id", unreal.Name(template_id))
    asset.set_editor_property("room_actor_class", ROOM_RUNTIME_CLASS)
    asset.set_editor_property("footprint", unreal.IntPoint(footprint[0], footprint[1]))
    asset.set_editor_property("connectors", connectors)
    asset.set_editor_property("allowed_room_types", make_container(room_type))
    asset.set_editor_property("generation_weight", weight)
    unreal.EditorAssetLibrary.save_loaded_asset(asset, False)
    return asset


def phase(index, rooms, normal_weight, monster_weight, high_resource_weight, start_time):
    value = unreal.SurvivalPhaseDefinition()
    value.set_editor_property("phase_index", index)
    value.set_editor_property("start_time_seconds", start_time)
    value.set_editor_property("rooms_to_unlock", rooms)
    value.set_editor_property("resource_budget", 10 + index * 5)
    value.set_editor_property("enemy_budget", index * 5)
    value.set_editor_property("max_alive_enemies", 4 + index * 2)
    value.set_editor_property("room_type_weights", {
        TAG_NORMAL: normal_weight,
        TAG_MONSTER: monster_weight,
        TAG_HIGH_RESOURCE: high_resource_weight,
    })
    return value


def configure_mode_config():
    asset = get_or_create_data_asset("DA_SurvivalMode_Default", MODE_CONFIG_CLASS, DATA_ROOT)
    asset.set_editor_property("random_seed", 1337)
    asset.set_editor_property("max_room_count", 32)
    asset.set_editor_property("min_team_start_graph_distance", 4)
    asset.set_editor_property("max_generation_attempts", 32)
    asset.set_editor_property("phases", [
        phase(0, 8, 1.0, 0.15, 0.10, 0.0),
        phase(1, 8, 0.65, 0.25, 0.10, 300.0),
        phase(2, 8, 0.50, 0.35, 0.15, 600.0),
        phase(3, 8, 0.35, 0.45, 0.20, 900.0),
    ])
    unreal.EditorAssetLibrary.save_loaded_asset(asset, False)
    return asset


def create_or_open_map():
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        unreal.EditorLevelLibrary.load_level(MAP_PATH)
        return True
    unreal.EditorLevelLibrary.new_level(MAP_PATH)
    return False


def add_map_actors(mode_config, start_room, room_templates):
    generator = unreal.EditorLevelLibrary.spawn_actor_from_class(GENERATOR_CLASS, unreal.Vector(0.0, 0.0, 0.0))
    generator.set_actor_label("SurvivalWorldGenerator")
    generator.set_editor_property("mode_config", mode_config)
    generator.set_editor_property("start_room_template", start_room)
    generator.set_editor_property("room_templates", room_templates)
    # Backrooms meshes use 300 uu modules, so six modules form one logical room cell.
    generator.set_editor_property("grid_cell_size", 1800.0)

    driver = unreal.EditorLevelLibrary.spawn_actor_from_class(TEST_DRIVER_CLASS, unreal.Vector(0.0, 0.0, 200.0))
    driver.set_actor_label("TEST_STUB_PhaseExpansionDriver")
    driver.set_editor_property("phase_interval_seconds", 10.0)

    nav_bounds = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(0.0, 0.0, 0.0))
    nav_bounds.set_actor_label("SurvivalDynamicNavBounds")
    nav_bounds.set_actor_scale3d(unreal.Vector(400.0, 400.0, 10.0))

    player_start = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(1000.0, 1000.0, 150.0))
    player_start.set_actor_label("FallbackPlayerStart")

    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_class().get_name() == "RecastNavMesh":
            try:
                actor.set_editor_property("runtime_generation", unreal.RuntimeGenerationType.DYNAMIC)
            except Exception as error:
                unreal.log_warning("Could not set Recast runtime generation: {}".format(error))


def remove_graybox_global_lights():
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    obsolete_labels = {"SurvivalDirectionalLight", "SurvivalSkyLight"}
    for actor in actor_subsystem.get_all_level_actors():
        if actor.get_actor_label() in obsolete_labels:
            actor_subsystem.destroy_actor(actor)


def configure_post_process():
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    volumes = [
        actor
        for actor in actor_subsystem.get_all_level_actors()
        if actor.get_class().get_name() == "PostProcessVolume"
    ]
    volume = volumes[0] if volumes else actor_subsystem.spawn_actor_from_class(
        unreal.PostProcessVolume, unreal.Vector(0.0, 0.0, 0.0)
    )
    if not volume:
        raise RuntimeError("Could not create Survival PostProcessVolume")
    volume.set_actor_label("SurvivalPostProcessVolume")
    volume.set_editor_property("unbound", True)
    settings = volume.get_editor_property("settings")
    settings.set_editor_property("override_bloom_intensity", True)
    settings.set_editor_property("bloom_intensity", 3.0)
    volume.set_editor_property("settings", settings)


def configure_generator(mode_config, start_room, room_templates):
    generators = [
        actor for actor in unreal.EditorLevelLibrary.get_all_level_actors()
        if actor.get_class() == GENERATOR_CLASS
    ]
    if generators:
        generator = generators[0]
        generator.set_editor_property("mode_config", mode_config)
        generator.set_editor_property("start_room_template", start_room)
        generator.set_editor_property("room_templates", room_templates)
        generator.set_editor_property("grid_cell_size", 1800.0)
        return
    add_map_actors(mode_config, start_room, room_templates)


def main():
    base = configure_room(
        "DA_Room_Base", "Survival.Base", (2, 2), 0.0, TAG_NORMAL,
        [connector("N0", 0, 1, NORTH), connector("N1", 1, 1, NORTH), connector("E0", 1, 0, EAST),
         connector("E1", 1, 1, EAST), connector("S0", 0, 0, SOUTH), connector("S1", 1, 0, SOUTH),
         connector("W0", 0, 0, WEST), connector("W1", 0, 1, WEST)],
    )
    normal_straight = configure_room(
        "DA_Room_NormalStraight", "Survival.NormalStraight", (1, 1), 1.0, TAG_NORMAL,
        [connector("N", 0, 0, NORTH), connector("S", 0, 0, SOUTH)],
    )
    normal_corner = configure_room(
        "DA_Room_NormalCorner", "Survival.NormalCorner", (1, 1), 1.0, TAG_NORMAL,
        [connector("N", 0, 0, NORTH), connector("E", 0, 0, EAST)],
    )
    normal_hall = configure_room(
        "DA_Room_NormalHall", "Survival.NormalHall", (2, 1), 0.8, TAG_NORMAL,
        [connector("W", 0, 0, WEST), connector("E", 1, 0, EAST)],
    )
    normal_pillar_hall = configure_room(
        "DA_Room_NormalPillarHall", "Survival.NormalPillarHall", (2, 2), 0.45, TAG_NORMAL,
        [connector("N", 0, 1, NORTH), connector("E", 1, 1, EAST),
         connector("S", 1, 0, SOUTH), connector("W", 0, 0, WEST)],
    )
    normal_partitioned = configure_room(
        "DA_Room_NormalPartitioned", "Survival.NormalPartitioned", (2, 2), 0.4, TAG_NORMAL,
        [connector("N", 1, 1, NORTH), connector("E", 1, 0, EAST),
         connector("S", 0, 0, SOUTH), connector("W", 0, 1, WEST)],
    )
    dark_corridor = configure_room(
        "DA_Room_DarkCorridor", "Survival.DarkCorridor", (1, 2), 0.3, TAG_NORMAL,
        [connector("S", 0, 0, SOUTH), connector("N", 0, 1, NORTH)],
    )
    monster = configure_room(
        "DA_Room_Monster", "Survival.Monster", (2, 2), 0.7, TAG_MONSTER,
        [connector("N0", 0, 1, NORTH), connector("N1", 1, 1, NORTH), connector("E0", 1, 0, EAST),
         connector("E1", 1, 1, EAST), connector("S0", 0, 0, SOUTH), connector("S1", 1, 0, SOUTH),
         connector("W0", 0, 0, WEST), connector("W1", 0, 1, WEST)],
    )
    high_resource = configure_room(
        "DA_Room_HighResource", "Survival.HighResource", (1, 2), 0.35, TAG_HIGH_RESOURCE,
        [connector("S", 0, 0, SOUTH), connector("N", 0, 1, NORTH)],
    )
    mode_config = configure_mode_config()
    create_or_open_map()
    configure_generator(
        mode_config,
        base,
        [
            base,
            normal_straight,
            normal_corner,
            normal_hall,
            normal_pillar_hall,
            normal_partitioned,
            dark_corridor,
            monster,
            high_resource,
        ],
    )
    remove_graybox_global_lights()
    configure_post_process()
    unreal.EditorLevelLibrary.save_current_level()
    unreal.log("Survival World assets generated successfully.")


main()
