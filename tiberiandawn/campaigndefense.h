// Player-only campaign defense targeting and SAM conveniences.
#ifndef CAMPAIGNDEFENSE_H
#define CAMPAIGNDEFENSE_H
inline bool Campaign_SAM_Ground(TechnoClass const* object)
{
    return GameToPlay == GAME_NORMAL && object && object->What_Am_I() == RTTI_BUILDING
        && object->House && object->House == PlayerPtr && object->House->IsHuman
        && static_cast<BuildingClass const*>(object)->Class->Type == STRUCT_SAM;
}
inline bool Campaign_SAM_Target_Allowed(BuildingClass const* sam, TARGET target)
{
    return Target_Legal(target) && (Campaign_SAM_Ground(sam)
        || (Is_Target_Aircraft(target) && As_Aircraft(target)->Altitude > 0));
}
inline bool Campaign_Defense_Targets_Buildings(TechnoClass const* object)
{
    if (GameToPlay != GAME_NORMAL || !object || object->What_Am_I() != RTTI_BUILDING
        || !object->House || object->House != PlayerPtr || !object->House->IsHuman) return false;
    StructType type = static_cast<BuildingClass const*>(object)->Class->Type;
    return type == STRUCT_ATOWER || type == STRUCT_OBELISK || type == STRUCT_SAM;
}
#endif
