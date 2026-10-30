#include "function.h"
#include "campaignbike.h"
#include "campaigndefense.h"
#include <array>
#include <map>
#include <vector>
#include <cstdint>
#include <cstring>
namespace {
std::map<TARGET,int> nextIon;
std::array<unsigned char,UNIT_MAX> healthBoost{};
const char magic[8]={'C','N','C','B','I','K','0','2'};
}
bool Campaign_Bike_Active(TechnoClass const* object)
{
    return GameToPlay==GAME_NORMAL && object && object->IsActive && !object->IsInLimbo && object->Strength>0
        && object->What_Am_I()==RTTI_UNIT && static_cast<UnitClass const*>(object)->Class->Type==UNIT_BIKE
        && object->House && object->House==PlayerPtr && object->House->IsHuman;
}
void Campaign_Bike_Reset(){nextIon.clear();healthBoost.fill(0);}
void Campaign_Bike_Forget(TARGET id){nextIon.erase(id);if(Is_Target_Unit(id)&&Target_Value(id)<UNIT_MAX)healthBoost[Target_Value(id)]=0;}
void Campaign_Bike_Health_Apply(UnitClass* bike)
{
    if(!bike||bike->Class->Type!=UNIT_BIKE)return;
    int id=Units.ID(bike);if(id<0||id>=UNIT_MAX)return;
    bool desired=Campaign_Player_Bike(bike);
    if(desired!=bool(healthBoost[id])){
        if(desired)bike->Strength=MIN(int(bike->Strength)*2,int(bike->Class->MaxStrength)*2);
        else if(bike->Strength>0)bike->Strength=MAX(1,(bike->Strength+1)/2);
        healthBoost[id]=desired;
    }
}
void Campaign_Bike_Health_Apply_All(){for(int i=0;i<Units.Count();++i)Campaign_Bike_Health_Apply(Units.Ptr(i));}
void Campaign_Bike_AI(UnitClass* bike)
{
    if(!GameActive || ScenarioInit || !Campaign_Bike_Active(bike) || PlayerWins || PlayerLoses) return;
    auto id=bike->As_Target();auto it=nextIon.find(id);
    if(it!=nextIon.end()&&Frame<it->second)return;
    nextIon[id]=Frame+TICKS_PER_SECOND;
    struct IonTarget {TechnoClass* object;COORDINATE coord;};
    std::vector<IonTarget> targets;
    auto hostile=[&](HouseClass const* house){return house&&house!=bike->House&&house->Class->House!=HOUSE_NEUTRAL
        &&!bike->House->Is_Ally(house)&&!house->Is_Ally(bike->House);};
    auto consider=[&](TechnoClass* target){
        if(!target||!target->IsActive||target->IsInLimbo||target->Strength<=0||!target->House
            ||!hostile(target->House)||!Map.In_Radar(Coord_Cell(target->Coord))||target->Is_Cloaked(bike->House)
            ||(!target->IsDiscoveredByPlayer&&target->What_Am_I()!=RTTI_AIRCRAFT
                &&!(target->What_Am_I()==RTTI_BUILDING&&Map[Coord_Cell(target->Coord)].Is_Visible(bike->House)))
            ||bike->Distance(target)>10*ICON_LEPTON_W)return;
        // Enemy-owned civilian/objective types participate even if normal weapon AI ignores them.
        targets.push_back(IonTarget{target,target->Target_Coord()});
    };
    for(int i=0;i<Units.Count();++i)consider(Units.Ptr(i));
    for(int i=0;i<Infantry.Count();++i)consider(Infantry.Ptr(i));
    for(int i=0;i<Buildings.Count();++i)consider(Buildings.Ptr(i));
    for(int i=0;i<Aircraft.Count();++i)consider(Aircraft.Ptr(i));
    // Walls are owned cell overlays, not Techno objects. Scan only the local range.
    CELL center=Coord_Cell(bike->Center_Coord());
    for(int y=MAX(Map.MapCellY,Cell_Y(center)-11);y<=MIN(Map.MapCellY+Map.MapCellHeight-1,Cell_Y(center)+11);++y)
        for(int x=MAX(Map.MapCellX,Cell_X(center)-11);x<=MIN(Map.MapCellX+Map.MapCellWidth-1,Cell_X(center)+11);++x){
            CELL c=XY_Cell(x,y);auto const& cell=Map[c];
            if(cell.Overlay==OVERLAY_NONE||cell.Owner<HOUSE_FIRST||cell.Owner>=HOUSE_COUNT
                ||!OverlayTypeClass::As_Reference(cell.Overlay).IsWall||!cell.Is_Visible(bike->House)
                ||!hostile(HouseClass::As_Pointer(cell.Owner))||::Distance(bike->Center_Coord(),Cell_Coord(c))>10*ICON_LEPTON_W)continue;
            targets.push_back(IonTarget{nullptr,Cell_Coord(c)});
        }
    if(targets.empty())return;
    auto choice=targets[Random_Pick(0,int(targets.size())-1)];
    auto target=choice.object;
    auto ion=new AnimClass(ANIM_ION_CANNON,choice.coord);
    if(!ion)return;
    ion->Set_Owner(bike->House->Class->House);
    // The original ion animation deals its normal 600 PB ground explosion.
    // Its ground explosion cannot reach airborne targets, so resolve that hit here.
    if(target&&target->What_Am_I()==RTTI_AIRCRAFT&&static_cast<AircraftClass*>(target)->Altitude>0){
        int damage=600;target->Take_Damage(damage,0,WARHEAD_PB,bike);
    }
}
bool Campaign_Bike_Save(FileClass& file)
{
    std::uint32_t count=nextIon.size();
    if(file.Write(magic,8)!=8||file.Write(&count,sizeof(count))!=sizeof(count))return false;
    for(auto e:nextIon){std::uint32_t data[2]={std::uint32_t(e.first),std::uint32_t(Bound(e.second-Frame,0,TICKS_PER_SECOND))};if(file.Write(data,sizeof(data))!=sizeof(data))return false;}
    return file.Write(healthBoost.data(),healthBoost.size())==int(healthBoost.size());
}
bool Campaign_Bike_Load(FileClass& file)
{
    Campaign_Bike_Reset();char header[8];int n=file.Read(header,8);if(!n)return true;
    const char oldMagic[8]={'C','N','C','B','I','K','0','1'};
    bool legacy=n==8&&!std::memcmp(header,oldMagic,8);
    if(n!=8||(!legacy&&std::memcmp(header,magic,8)))return false;
    std::uint32_t count;if(file.Read(&count,sizeof(count))!=sizeof(count)||count>UNIT_MAX)return false;
    for(unsigned i=0;i<count;++i){std::uint32_t data[2];if(file.Read(data,sizeof(data))!=sizeof(data)||!Is_Target_Unit(data[0])||Target_Value(data[0])>=UNIT_MAX||data[1]>TICKS_PER_SECOND||nextIon.count(data[0]))return false;nextIon[data[0]]=Frame+data[1];}
    if(!legacy){if(file.Read(healthBoost.data(),healthBoost.size())!=int(healthBoost.size()))return false;for(auto flag:healthBoost)if(flag>1)return false;}
    return true;
}
