#include "function.h"
#include "campaignbike.h"
#include <map>
#include <vector>
#include <cstdint>
#include <cstring>
namespace {
std::map<TARGET,int> nextIon;
const char magic[8]={'C','N','C','B','I','K','0','1'};
}
bool Campaign_Bike_Active(TechnoClass const* object)
{
    return GameToPlay==GAME_NORMAL && object && object->IsActive && !object->IsInLimbo && object->Strength>0
        && object->What_Am_I()==RTTI_UNIT && static_cast<UnitClass const*>(object)->Class->Type==UNIT_BIKE
        && object->House && object->House==PlayerPtr && object->House->IsHuman;
}
void Campaign_Bike_Reset(){nextIon.clear();}
void Campaign_Bike_Forget(TARGET id){nextIon.erase(id);}
void Campaign_Bike_AI(UnitClass* bike)
{
    if(!GameActive || ScenarioInit || !Campaign_Bike_Active(bike) || PlayerWins || PlayerLoses) return;
    auto id=bike->As_Target();auto it=nextIon.find(id);
    if(it!=nextIon.end()&&Frame<it->second)return;
    nextIon[id]=Frame+TICKS_PER_SECOND;
    std::vector<TechnoClass*> targets;
    auto consider=[&](TechnoClass* target){
        if(!target||!target->IsActive||target->IsInLimbo||target->Strength<=0||!target->House
            ||target->House==bike->House||target->House->Class->House==HOUSE_NEUTRAL
            ||bike->House->Is_Ally(target->House)||target->House->Is_Ally(bike->House)
            ||!target->Techno_Type_Class()->IsLegalTarget||target->Is_Cloaked(bike->House)
            ||(!target->IsDiscoveredByPlayer&&target->What_Am_I()!=RTTI_AIRCRAFT)
            ||!Map.In_Radar(Coord_Cell(target->Coord))||bike->Distance(target)>10*ICON_LEPTON_W)return;
        if(target->What_Am_I()==RTTI_INFANTRY&&*static_cast<InfantryClass*>(target)==INFANTRY_DELPHI)return;
        targets.push_back(target);
    };
    for(int i=0;i<Units.Count();++i)consider(Units.Ptr(i));
    for(int i=0;i<Infantry.Count();++i)consider(Infantry.Ptr(i));
    for(int i=0;i<Buildings.Count();++i)consider(Buildings.Ptr(i));
    for(int i=0;i<Aircraft.Count();++i)consider(Aircraft.Ptr(i));
    if(targets.empty())return;
    auto target=targets[Random_Pick(0,int(targets.size())-1)];
    auto ion=new AnimClass(ANIM_ION_CANNON,target->Target_Coord());
    if(!ion)return;
    ion->Set_Owner(bike->House->Class->House);
    // The original ion animation deals its normal 600 PB ground explosion.
    // Its ground explosion cannot reach airborne targets, so resolve that hit here.
    if(target->What_Am_I()==RTTI_AIRCRAFT&&static_cast<AircraftClass*>(target)->Altitude>0){
        int damage=600;target->Take_Damage(damage,0,WARHEAD_PB,bike);
    }
}
bool Campaign_Bike_Save(FileClass& file)
{
    std::uint32_t count=nextIon.size();
    if(file.Write(magic,8)!=8||file.Write(&count,sizeof(count))!=sizeof(count))return false;
    for(auto e:nextIon){std::uint32_t data[2]={std::uint32_t(e.first),std::uint32_t(Bound(e.second-Frame,0,TICKS_PER_SECOND))};if(file.Write(data,sizeof(data))!=sizeof(data))return false;}
    return true;
}
bool Campaign_Bike_Load(FileClass& file)
{
    Campaign_Bike_Reset();char header[8];int n=file.Read(header,8);if(!n)return true;
    if(n!=8||std::memcmp(header,magic,8))return false;
    std::uint32_t count;if(file.Read(&count,sizeof(count))!=sizeof(count)||count>UNIT_MAX)return false;
    for(unsigned i=0;i<count;++i){std::uint32_t data[2];if(file.Read(data,sizeof(data))!=sizeof(data)||!Is_Target_Unit(data[0])||Target_Value(data[0])>=UNIT_MAX||data[1]>TICKS_PER_SECOND||nextIon.count(data[0]))return false;nextIon[data[0]]=Frame+data[1];}
    return true;
}
