// Mobilize existing campaign troops; retain compatibility with retired wave saves.
#include "function.h"
#include "campaignwaves.h"
#include "campaignenemies.h"
#include "campaigndefense.h"
#include <cstdint>
#include <cstring>
namespace {
const char magic[8] = {'C','N','C','W','A','V','0','1'};
bool Hostile(HouseClass* house)
{
    return house && house->IsActive && !house->IsDefeated && house != PlayerPtr && !house->IsHuman
        && house->Class->House != HOUSE_NEUTRAL
        && !house->Is_Ally(PlayerPtr) && !PlayerPtr->Is_Ally(house);
}
TechnoClass* PlayerAnchor()
{
    for (int i=0;i<Buildings.Count();++i) {
        auto b=Buildings.Ptr(i);
        if (!b->IsInLimbo && b->Strength>0 && b->House==PlayerPtr) return b;
    }
    for (int i=0;i<Units.Count();++i) {
        auto u=Units.Ptr(i);
        if (!u->IsInLimbo && u->Strength>0 && u->House==PlayerPtr && Map.In_Radar(Coord_Cell(u->Coord))) return u;
    }
    for (int i=0;i<Infantry.Count();++i) {
        auto f=Infantry.Ptr(i);
        if (!f->IsInLimbo && f->Strength>0 && f->House==PlayerPtr && Map.In_Radar(Coord_Cell(f->Coord))) return f;
    }
    return nullptr;
}
void PressAttack(FootClass* object,TechnoClass* base,int quarters)
{
    if (!object || !object->IsActive || object->IsInLimbo || object->Strength<=0
        || !Hostile(object->House) || object->Team || object->Trigger || object->IsALoaner || object->In_Radio_Contact()
        || !Map.In_Radar(Coord_Cell(object->Coord)) || Target_Value(object->As_Target())%8>=quarters) return;
    auto type=object->Techno_Type_Class();
    if (type->Primary==WEAPON_NONE || type->IsTransporter) return;
    if (object->What_Am_I()==RTTI_UNIT) {
        auto unit=static_cast<UnitClass*>(object);
        if (unit->Class->IsToHarvest || unit->Class->Type==UNIT_GUNBOAT || unit->Class->Type==UNIT_HOVER) return;
    }
    switch (object->Mission) {
    case MISSION_GUARD: case MISSION_GUARD_AREA: case MISSION_AMBUSH:
    case MISSION_STICKY: case MISSION_SLEEP: case MISSION_HUNT: break;
    default: return; // Preserve movement, transport, repair and scripted attack orders.
    }
    auto current=As_Techno(object->TarCom);
    if (object->Mission==MISSION_HUNT && current && current->House==PlayerPtr) return;
    auto target=current && current->House==PlayerPtr ? current : base;
    object->Assign_Mission(MISSION_HUNT);object->Commence();
    object->Assign_Target(target->As_Target());object->Assign_Destination(target->As_Target());
}
void Mobilize()
{
    if (Frame<30*TICKS_PER_SECOND) return;
    int seconds=5;
    if (Frame%(seconds*TICKS_PER_SECOND)) return;
    auto target=PlayerAnchor();if (!target) return;
    int quarters=4;
    for (int i=0;i<Units.Count();++i) PressAttack(Units.Ptr(i),target,quarters);
    for (int i=0;i<Infantry.Count();++i) PressAttack(Infantry.Ptr(i),target,quarters);
}
}
void Campaign_Waves_Reset() { Campaign_SAM_Volley_Reset(); }
void Campaign_Waves_AI()
{
    if (GameToPlay!=GAME_NORMAL || !GameActive || ScenarioInit || !PlayerPtr || !PlayerPtr->IsHuman
        || PlayerWins || PlayerLoses || PlayerPtr->IsToWin || PlayerPtr->IsToLose || PlayerPtr->IsToDie) return;
    Mobilize(); // Existing troops only. No time-based spawning or escalating waves.
}
bool Campaign_Waves_Save(FileClass& file)
{
    std::int32_t data[7]={-1,0,0,0,HOUSE_NONE,0,-1};
    return file.Write(magic,sizeof(magic))==sizeof(magic) && file.Write(data,sizeof(data))==sizeof(data) && Campaign_SAM_Volley_Save(file);
}
bool Campaign_Waves_Load(FileClass& file)
{
    Campaign_SAM_Volley_Reset();Campaign_SAM_Health_Reset();
    char header[8];int bytes=file.Read(header,8);
    if (!bytes) return true;
    if (bytes!=8||std::memcmp(header,magic,8)) return false;
    std::int32_t data[7];if (file.Read(data,sizeof(data))!=sizeof(data)) return false;
    if (data[0]<-1||data[1]<0||data[2]<0||data[2]>UNIT_MAX+INFANTRY_MAX||data[3]<0||data[3]>UNIT_MAX+INFANTRY_MAX
        ||data[2]+data[3]>UNIT_MAX+INFANTRY_MAX||data[4]<HOUSE_NONE||data[4]>=HOUSE_COUNT
        ||(data[2]>0&&(data[4]==HOUSE_NONE||data[5]==0))||data[5]<0||data[5]>=MAP_CELL_TOTAL
        ||(data[6]!=-1&&(data[6]<UNIT_TRIC||data[6]>UNIT_STEG))) return false;
    return Campaign_SAM_Volley_Load(file); // Legacy scheduled arrivals are deliberately discarded.
}
