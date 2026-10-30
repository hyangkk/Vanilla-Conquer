// Runtime bookkeeping stays outside serialized game object layouts.
#include "function.h"
#include "siegetank.h"
#include "campaigndefense.h"
#include "campaignenemies.h"
#include "campaignwaves.h"
#include "campaignbike.h"
#include "campaigntraffic.h"
#include "campaignformation.h"
#include <limits>
#include <cstdint>
#include <cstring>
#include <set>
#include <map>
#include <vector>
namespace {
std::set<TARGET> processed;
struct Companion {
    TARGET extra, target, destination;
    MissionType mission;
};
std::multimap<TARGET, Companion> companions;
std::map<TARGET,unsigned> pending;
FootClass* Foot(TARGET target)
{
    auto object = As_Techno(target);
    if (!object || !object->IsActive) return nullptr;
    switch(object->What_Am_I()) {
    case RTTI_UNIT: case RTTI_INFANTRY: case RTTI_AIRCRAFT: return static_cast<FootClass*>(object);
    default: return nullptr;
    }
}
void SyncOrders()
{
    for (auto& entry : companions) {
        auto original = Foot(entry.first);
        auto extra = Foot(entry.second.extra);
        if (!original || !extra || original->IsInLimbo || extra->IsInLimbo
            || original->House != extra->House) continue;
        if(original->SuspendedMission!=MISSION_NONE||extra->SuspendedMission!=MISSION_NONE
            ||Campaign_Formation_Holding(original)||Campaign_Formation_Holding(extra))continue;
        auto& last = entry.second;
        if (original->Mission != last.mission) {
            extra->Assign_Mission(original->Mission); extra->Commence(); last.mission=original->Mission;
        }
        if (original->TarCom != last.target) {
            extra->Assign_Target(original->TarCom); last.target=original->TarCom;
        }
        if (original->NavCom != last.destination) {
            extra->Assign_Destination(original->NavCom); last.destination=original->NavCom;
        }
    }
}
const char magic[8] = {'C','N','C','E','N','M','0','2'};
bool Eligible(FootClass* object)
{
    return object && object->IsActive && !object->IsInLimbo && object->Strength > 0
        && object->House && object->House != PlayerPtr && !object->House->IsHuman
        && object->House->Class->House != HOUSE_NEUTRAL
        && !PlayerPtr->Is_Ally(object->House) && !object->House->Is_Ally(PlayerPtr)
        && Map.In_Radar(Coord_Cell(object->Coord));
}
bool Duplicate(FootClass* original)
{
    FootClass* extra = static_cast<FootClass*>(original->Class_Of().Create_One_Of(original->House));
    if (!extra) return false; // Heap full: retry after losses free space.
    if (original->What_Am_I() == RTTI_AIRCRAFT) {
        static_cast<AircraftClass*>(extra)->Altitude = static_cast<AircraftClass*>(original)->Altitude;
    }
    CELL center = Coord_Cell(original->Coord);
    bool placed = false;
    for (int radius = 0; radius <= 6 && !placed; ++radius) {
        for (int dy = -radius; dy <= radius && !placed; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                if (MAX(ABS(dx), ABS(dy)) != radius) continue;
                int x = Cell_X(center) + dx, y = Cell_Y(center) + dy;
                if (x < 0 || x >= MAP_CELL_W || y < 0 || y >= MAP_CELL_H) continue;
                CELL cell = XY_Cell(x, y);
                if (!Map.In_Radar(cell) || extra->Can_Enter_Cell(cell) != MOVE_OK) continue;
                if (extra->Unlimbo(Cell_Coord(cell), original->PrimaryFacing.Current())) {
                    placed = true;
                    break;
                }
            }
        }
    }
    if (!placed) { delete extra; return false; }
    extra->Strength = original->Strength;
    extra->Ammo = original->Ammo;
    extra->IsALoaner = original->IsALoaner;
    // Never copy mission triggers, transport cargo, radio links or team pointers.
    // Those belong to the original scripted actor; the companion follows its orders.
    extra->Assign_Mission(original->Mission);
    extra->Commence();
    extra->Assign_Target(original->TarCom);
    extra->Assign_Destination(original->NavCom);
    processed.insert(extra->As_Target());
    companions.emplace(original->As_Target(),Companion{extra->As_Target(), original->TarCom, original->NavCom, original->Mission});
    if(Campaign_Formation_Holding(original))Campaign_Formation_Recruit(extra,true);
    return true;
}
}
void Campaign_Enemies_Reset() { SiegeTank::Reset(); Campaign_Traffic_Reset();Campaign_Formation_Reset();processed.clear(); companions.clear();pending.clear();Campaign_Bike_Reset(); Campaign_Waves_Reset(); Campaign_SAM_Health_Reset(); Campaign_SAM_Chain_Reset(); }
void Campaign_Enemies_Mark_Extra(const TechnoClass* object) { processed.insert(object->As_Target()); }
void Campaign_Enemies_Forget(const TechnoClass* object) {
    TARGET id=object->As_Target(); Campaign_Traffic_Forget(id);Campaign_Formation_Forget(id);processed.erase(id); companions.erase(id);pending.erase(id);Campaign_Bike_Forget(id);
    for (auto it=companions.begin(); it!=companions.end();) {
        if (it->second.extra==id) it=companions.erase(it); else ++it;
    }
}
void Campaign_Enemies_Apply()
{
    if (GameToPlay != GAME_NORMAL || !GameActive || !PlayerPtr || !PlayerPtr->IsHuman || ScenarioInit) return;
    for (int i=0;i<Houses.Count();++i) {
        auto h=Houses.Ptr(i);
        if (Campaign_Enemy_House(h) && (h->ActiveBScan&(STRUCTF_WEAP|STRUCTF_HAND|STRUCTF_BARRACKS|STRUCTF_AIRSTRIP|STRUCTF_HELIPAD))) {
            h->IsStarted=true;h->IsAlerted=true;
        }
    }
    for (int i=0;i<Factories.Count();++i) {
        auto obj=Factories.Ptr(i)->Get_Object();
        if (obj && !processed.count(obj->As_Target()) && !pending.count(obj->As_Target())) Campaign_Enemies_Produced(obj);
    }
    SyncOrders();
    std::vector<FootClass*> originals;
    for (int i=0;i<Units.Count();++i) originals.push_back(Units.Ptr(i));
    for (int i=0;i<Infantry.Count();++i) originals.push_back(Infantry.Ptr(i));
    for (int i=0;i<Aircraft.Count();++i) originals.push_back(Aircraft.Ptr(i));
    for (auto object : originals) {
        if (Eligible(object) && !processed.count(object->As_Target())) {
            TARGET id=object->As_Target();unsigned left=pending.count(id)?pending[id]:1;
            while (left && Duplicate(object)) --left;
            if (left) pending[id]=left;
            else {processed.insert(id);pending.erase(id);}
        }
    }
}
bool Campaign_Enemies_Save(FileClass& file)
{
    std::uint32_t count = processed.size();
    if (file.Write(magic, sizeof(magic)) != sizeof(magic) || file.Write(&count, sizeof(count)) != sizeof(count)) return false;
    for (TARGET target : processed) {
        std::uint32_t id = target;
        if (file.Write(&id, sizeof(id)) != sizeof(id)) return false;
    }
    count = companions.size();
    if (file.Write(&count,sizeof(count)) != sizeof(count)) return false;
    for (auto const& entry : companions) {
        std::uint32_t data[5] = {std::uint32_t(entry.first), std::uint32_t(entry.second.extra),
            std::uint32_t(entry.second.target), std::uint32_t(entry.second.destination), std::uint32_t(entry.second.mission)};
        if (file.Write(data,sizeof(data)) != sizeof(data)) return false;
    }
    count=pending.size();if(file.Write(&count,sizeof(count))!=sizeof(count)) return false;
    for(auto entry:pending){std::uint32_t data[2]={std::uint32_t(entry.first),entry.second};if(file.Write(data,sizeof(data))!=sizeof(data))return false;}
    return Campaign_Waves_Save(file) && Campaign_Bike_Save(file) && Campaign_Formation_Save(file) && SiegeTank::Save(file);
}
bool Campaign_Enemies_Load(FileClass& file)
{
    Campaign_Enemies_Reset();
    char header[sizeof(magic)];
    int bytes = file.Read(header, sizeof(header));
    if (!bytes) return true; // Native old saves have no optional footer.
    const char oldMagic[8]={'C','N','C','E','N','M','0','1'};
    bool legacy=bytes==8&&!std::memcmp(header,oldMagic,8);
    if (bytes != sizeof(header) || (!legacy&&std::memcmp(header, magic, sizeof(magic)))) return false;
    std::uint32_t count;
    if (file.Read(&count, sizeof(count)) != sizeof(count) || count > UNIT_MAX+INFANTRY_MAX+AIRCRAFT_MAX) return false;
    for (std::uint32_t i=0; i<count; ++i) {
        std::uint32_t id;
        if (file.Read(&id, sizeof(id)) != sizeof(id)) return false;
        processed.insert(TARGET(id));
    }
    if (file.Read(&count,sizeof(count)) != sizeof(count) || count > UNIT_MAX+INFANTRY_MAX+AIRCRAFT_MAX) return false;
    for (std::uint32_t i=0;i<count;++i) {
        std::uint32_t data[5];
        if (file.Read(data,sizeof(data)) != sizeof(data)) return false;
        if (data[4] != std::uint32_t(MISSION_NONE) && data[4] >= MISSION_COUNT) return false;
        companions.emplace(TARGET(data[0]),Companion{TARGET(data[1]),TARGET(data[2]),TARGET(data[3]),MissionType(data[4])});
    }
    if(!legacy){
        if(file.Read(&count,sizeof(count))!=sizeof(count)||count>UNIT_MAX+INFANTRY_MAX+AIRCRAFT_MAX)return false;
        for(unsigned i=0;i<count;++i){std::uint32_t data[2];if(file.Read(data,sizeof(data))!=sizeof(data)||data[1]<1||data[1]>2||pending.count(data[0])
            ||(!Is_Target_Unit(data[0])&&!Is_Target_Infantry(data[0])&&!Is_Target_Aircraft(data[0])))return false;pending[data[0]]=data[1];}
    }
    return Campaign_Waves_Load(file) && Campaign_Bike_Load(file) && Campaign_Formation_Load(file) && SiegeTank::Load(file);
}

bool Campaign_Enemy_House(HouseClass const* house)
{
    return GameToPlay==GAME_NORMAL && PlayerPtr && PlayerPtr->IsHuman && house && house->IsActive && !house->IsHuman
        && house!=PlayerPtr && !house->IsDefeated && house->Class->House!=HOUSE_NEUTRAL
        && !house->Is_Ally(PlayerPtr) && !PlayerPtr->Is_Ally(house);
}
void Campaign_Enemies_Produced(TechnoClass* object)
{
    if(!object||!Campaign_Enemy_House(object->House))return;
    auto kind=object->What_Am_I();if(kind!=RTTI_UNIT&&kind!=RTTI_INFANTRY&&kind!=RTTI_AIRCRAFT)return;
    if(!processed.count(object->As_Target())&&!pending.count(object->As_Target()))pending[object->As_Target()]=2;
}
void Campaign_Enemy_Harvest(HouseClass* house,unsigned credits)
{
    if(!Campaign_Enemy_House(house)){house->Harvested(credits);return;}
    // Deposit as liquid credits so silos cannot silently discard the 10x yield.
    std::int64_t cap=std::numeric_limits<int>::max()-MAX(0,house->Tiberium);
    house->Credits=int(std::min(cap,std::int64_t(house->Credits)+10*std::int64_t(credits)));
    house->HarvestedCredits=int(std::min(std::int64_t(std::numeric_limits<int>::max()),std::int64_t(house->HarvestedCredits)+10*std::int64_t(credits)));
}
TechnoTypeClass const* Campaign_Enemy_Suggest(HouseClass const* house,RTTIType kind)
{
    if(!Campaign_Enemy_House(house))return nullptr;
    std::vector<TechnoTypeClass const*> choices;
    auto consider=[&](TechnoTypeClass const& type){if(type.Primary!=WEAPON_NONE&&house->Can_Build(&type,house->Class->House)&&type.Cost_Of()<=house->Available_Money())choices.push_back(&type);};
    if((kind==RTTI_UNIT||kind==RTTI_UNITTYPE)&&house->CurUnits<house->MaxUnit){
        if((house->ActiveBScan&STRUCTF_REFINERY)&&!(house->UScan&UNITF_HARVESTER)){
            auto& h=UnitTypeClass::As_Reference(UNIT_HARVESTER);if(house->Can_Build(&h,house->Class->House)&&h.Cost_Of()<=house->Available_Money())return &h;
        }
        for(int i=UNIT_FIRST;i<UNIT_COUNT;++i)consider(UnitTypeClass::As_Reference(UnitType(i)));
    }
    if((kind==RTTI_INFANTRY||kind==RTTI_INFANTRYTYPE)&&house->CurInfantry<house->MaxInfantry)
        for(int i=INFANTRY_FIRST;i<INFANTRY_COUNT;++i)consider(InfantryTypeClass::As_Reference(InfantryType(i)));
    if((kind==RTTI_AIRCRAFT||kind==RTTI_AIRCRAFTTYPE)&&house->CurAircraft<house->MaxAircraft)
        for(int i=AIRCRAFT_FIRST;i<AIRCRAFT_COUNT;++i)consider(AircraftTypeClass::As_Reference(AircraftType(i)));
    return choices.empty()?nullptr:choices[Random_Pick(0,int(choices.size())-1)];
}
