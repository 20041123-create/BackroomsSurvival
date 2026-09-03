#include "SurvivalGameplayTags.h"

namespace LG::SurvivalTags
{
	UE_DEFINE_GAMEPLAY_TAG(Room_Type_Normal, "Room.Type.Normal");
	UE_DEFINE_GAMEPLAY_TAG(Room_Type_Monster, "Room.Type.Monster");
	UE_DEFINE_GAMEPLAY_TAG(Room_Type_HighResource, "Room.Type.HighResource");

	UE_DEFINE_GAMEPLAY_TAG(Anchor_Resource, "Anchor.Resource");
	UE_DEFINE_GAMEPLAY_TAG(Anchor_Enemy, "Anchor.Enemy");
	UE_DEFINE_GAMEPLAY_TAG(Anchor_Workbench, "Anchor.Workbench");
	UE_DEFINE_GAMEPLAY_TAG(Anchor_TeamTerminal, "Anchor.TeamTerminal");
	UE_DEFINE_GAMEPLAY_TAG(Anchor_RespawnBase, "Anchor.RespawnBase");
	UE_DEFINE_GAMEPLAY_TAG(Anchor_PlayerStart, "Anchor.PlayerStart");

	UE_DEFINE_GAMEPLAY_TAG(Item_Weapon, "Item.Category.Weapon");
	UE_DEFINE_GAMEPLAY_TAG(Item_Ammo, "Item.Category.Ammo");
	UE_DEFINE_GAMEPLAY_TAG(Item_Armor, "Item.Category.Armor");
	UE_DEFINE_GAMEPLAY_TAG(Item_Food, "Item.Category.Food");
	UE_DEFINE_GAMEPLAY_TAG(Item_Water, "Item.Category.Water");
	UE_DEFINE_GAMEPLAY_TAG(Item_Medical, "Item.Category.Medical");
	UE_DEFINE_GAMEPLAY_TAG(Item_Material, "Item.Category.Material");
	UE_DEFINE_GAMEPLAY_TAG(Item_RespawnEnergy, "Item.Category.RespawnEnergy");
}
