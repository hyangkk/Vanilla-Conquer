// Player-only campaign defenses may automatically engage enemy structures.
#ifndef CAMPAIGNDEFENSE_H
#define CAMPAIGNDEFENSE_H
inline bool Campaign_Defense_Targets_Buildings(TechnoClass const* object)
{
    if (GameToPlay != GAME_NORMAL || !object || object->What_Am_I() != RTTI_BUILDING
        || !object->House || object->House != PlayerPtr || !object->House->IsHuman) return false;
    StructType type = static_cast<BuildingClass const*>(object)->Class->Type;
    return type == STRUCT_ATOWER || type == STRUCT_OBELISK;
}
#endif
