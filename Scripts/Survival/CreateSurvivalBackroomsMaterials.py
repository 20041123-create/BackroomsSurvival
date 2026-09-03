import unreal


DESTINATION = "/Game/LegoGame/Survival/Materials"
MATERIAL_COPIES = {
    "/Game/BackRooms/carpet": "M_Survival_Backrooms_Carpet",
    "/Game/BackRooms/linoleum": "M_Survival_Backrooms_Linoleum",
    "/Game/BackRooms/ceiling_panels": "M_Survival_Backrooms_CeilingPanels",
    "/Game/BackRooms/ceiling_frame": "M_Survival_Backrooms_CeilingFrame",
    "/Game/BackRooms/ceiling_vent": "M_Survival_Backrooms_CeilingVent",
    "/Game/BackRooms/lamp": "M_Survival_Backrooms_Lamp",
    "/Game/BackRooms/wallpaper": "M_Survival_Backrooms_Wallpaper",
    "/Game/BackRooms/plaster": "M_Survival_Backrooms_Plaster",
    "/Game/BackRooms/plaster_white": "M_Survival_Backrooms_PlasterWhite",
    "/Game/BackRooms/wall_trim": "M_Survival_Backrooms_WallTrim",
    "/Game/BackRooms/wall_trim_dark": "M_Survival_Backrooms_WallTrimDark",
}


asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
unreal.EditorAssetLibrary.make_directory(DESTINATION)

for source_path, destination_name in MATERIAL_COPIES.items():
    destination_path = "{}/{}".format(DESTINATION, destination_name)
    material = unreal.load_asset(destination_path)
    was_created = False
    if not material:
        source_material = unreal.load_asset(source_path)
        if not source_material:
            raise RuntimeError("Missing Backrooms source material: {}".format(source_path))
        material = asset_tools.duplicate_asset(destination_name, DESTINATION, source_material)
        was_created = True
    if not material:
        raise RuntimeError("Failed to create Survival material: {}".format(destination_path))
    if not material.get_editor_property("used_with_instanced_static_meshes"):
        material.set_editor_property("used_with_instanced_static_meshes", True)
        unreal.MaterialEditingLibrary.recompile_material(material)
        was_created = True
    if not material.get_editor_property("used_with_instanced_static_meshes"):
        raise RuntimeError("Instanced-static-mesh usage was not enabled: {}".format(destination_path))
    if was_created and not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=True):
        raise RuntimeError("Failed to save Survival material: {}".format(destination_path))
    unreal.log("SURVIVAL_BACKROOMS_MATERIAL {}".format(destination_path))
