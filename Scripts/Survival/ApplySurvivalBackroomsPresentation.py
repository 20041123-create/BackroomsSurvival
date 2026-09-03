import unreal


MAP_PATH = "/Game/LegoGame/Survival/Maps/L_SurvivalWorld"


def main():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        class_name = actor.get_class().get_name()
        if class_name == "SurvivalWorldGenerator":
            # Six 300 uu Backrooms modules per logical layout cell.
            actor.set_editor_property("grid_cell_size", 1800.0)
        elif class_name == "NavMeshBoundsVolume" and actor.get_actor_label() == "SurvivalDynamicNavBounds":
            actor.set_actor_location(unreal.Vector(0.0, 0.0, 0.0), False, False)
            actor.set_actor_scale3d(unreal.Vector(400.0, 400.0, 10.0))
        elif class_name == "RecastNavMesh":
            actor.set_editor_property("runtime_generation", unreal.RuntimeGenerationType.DYNAMIC)
    unreal.EditorLevelLibrary.save_current_level()
    unreal.log("Applied Backrooms presentation settings to {}".format(MAP_PATH))


main()
