// Player-only campaign defense targeting and SAM conveniences.
#ifndef CAMPAIGNDEFENSE_H
#define CAMPAIGNDEFENSE_H
inline bool Campaign_SAM_Ground(TechnoClass const* object)
{
    return GameToPlay == GAME_NORMAL && object && object->What_Am_I() == RTTI_BUILDING
        && object->House && object->House == PlayerPtr && object->House->IsHuman
        && static_cast<BuildingClass const*>(object)->Class->Type == STRUCT_SAM;
}
// Integer simulation ticks round down, with at least one tick per state.
inline int Campaign_SAM_Attack_Delay(TechnoClass const* object, int ticks)
{
    return Campaign_SAM_Ground(object) ? MAX(1, ticks / 2) : ticks;
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
inline int Campaign_Defense_Rearm_Delay(TechnoClass const* object,int ticks,bool roundUp=false)
{
    if (Campaign_SAM_Ground(object)) return Campaign_SAM_Attack_Delay(object,ticks);
    return Campaign_Defense_Targets_Buildings(object) ? MAX(1,(ticks+(roundUp?1:0))/2) : ticks;
}
void Campaign_SAM_Explosion_Damage(COORDINATE coord, unsigned strength, TechnoClass* source, WarheadType warhead, bool airborne);
void Campaign_SAM_Volley(BuildingClass* sam, TARGET target, int weapon);
void Campaign_SAM_Volley_Reset();
void Campaign_SAM_Volley_Forget(BuildingClass* sam);
bool Campaign_SAM_Volley_Save(FileClass& file);
bool Campaign_SAM_Volley_Load(FileClass& file);
inline int Campaign_Max_Health(ObjectClass const* object)
{
    int maximum=object->Class_Of().MaxStrength;
    if (object->What_Am_I()==RTTI_BUILDING && Campaign_SAM_Ground(static_cast<BuildingClass const*>(object))) maximum*=5;
    return maximum;
}
void Campaign_SAM_Health_Reset();
void Campaign_SAM_Health_Apply(BuildingClass* sam);
void Campaign_SAM_Health_Apply_All();
#endif
