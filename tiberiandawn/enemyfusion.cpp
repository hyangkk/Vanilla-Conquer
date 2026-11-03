// Per-object evolution lives outside native object/save layouts.
#include "function.h"
#include "enemyfusion.h"
#include "campaignenemies.h"
#include "campaigndefense.h"
#include "siegetank.h"
#include <map>
#include <vector>
#include <set>
#include <cstdint>
#include <cstring>
namespace EnemyFusion {
namespace {
std::map<TARGET, unsigned> tiers;
const char magic[8]={'C','N','C','F','U','S','0','1'};
const unsigned maxTier=15; // Native health and projectile strength are signed 16-bit.
bool Mobile(ObjectClass const* o) {
    return o && (o->What_Am_I()==RTTI_UNIT || o->What_Am_I()==RTTI_INFANTRY || o->What_Am_I()==RTTI_AIRCRAFT);
}
bool Ready(FootClass* o) {
    return Mobile(o) && o->IsActive && !o->IsInLimbo && o->Strength>0 && Campaign_Enemy_House(o->House)
        && Map.In_Radar(Coord_Cell(o->Coord)) && !o->IsTethered && !o->In_Radio_Contact()
        && !o->Is_Something_Attached() && !o->Tiberium_Load()
        && (o->What_Am_I()!=RTTI_UNIT || !SiegeTank::Transitioning(static_cast<UnitClass*>(o)));
}
bool Same(FootClass* a,FootClass* b) {
    return a->House==b->House && &a->Class_Of()==&b->Class_Of() && Tier(a)==Tier(b)
        && (a->What_Am_I()!=RTTI_AIRCRAFT ||
            (static_cast<AircraftClass*>(a)->Altitude>0)==(static_cast<AircraftClass*>(b)->Altitude>0));
}
bool ValidID(TARGET id) {
    return (Is_Target_Unit(id) && Target_Value(id)<UNIT_MAX)
        || (Is_Target_Infantry(id) && Target_Value(id)<INFANTRY_MAX)
        || (Is_Target_Aircraft(id) && Target_Value(id)<AIRCRAFT_MAX);
}
}
int Tier(ObjectClass const* object) {
    if(!Mobile(object))return 0;
    auto it=tiers.find(object->As_Target());return it==tiers.end()?0:it->second;
}
int Multiplier(ObjectClass const* object) {return 1<<Tier(object);}
int Health(ObjectClass const* object,int base) {return int(std::min(std::int64_t(32767),std::int64_t(base)*Multiplier(object)));}
int Firepower(TechnoClass const* object,int base) {return Health(object,base);}
void Forget(ObjectClass const* object) {if(Mobile(object))tiers.erase(object->As_Target());}
void Reset(){tiers.clear();}
void Redraw(){if(!tiers.empty())Map.Flag_To_Redraw(true);}
void AI() {
    if(!GameActive || ScenarioInit || GameToPlay!=GAME_NORMAL || !PlayerPtr || !PlayerPtr->IsHuman)return;
    std::vector<TARGET> objects;
    for(int i=0;i<Units.Count();++i)objects.push_back(Units.Ptr(i)->As_Target());
    for(int i=0;i<Infantry.Count();++i)objects.push_back(Infantry.Ptr(i)->As_Target());
    for(int i=0;i<Aircraft.Count();++i)objects.push_back(Aircraft.Ptr(i)->As_Target());
    std::set<TARGET> consumed;
    for(auto anchorID:objects) {
        if(consumed.count(anchorID))continue;
        auto anchor=static_cast<FootClass*>(As_Techno(anchorID));
        if(!anchor || !Ready(anchor) || Tier(anchor)>=int(maxTier))continue;
        std::vector<FootClass*> group{anchor};
        for(auto otherID:objects) {
            if(consumed.count(otherID))continue;
            auto other=static_cast<FootClass*>(As_Techno(otherID));
            if(!other || other==anchor || !Ready(other) || !Same(anchor,other))continue;
            // Mission-linked actors cannot be discarded; an anchor keeps its own trigger and orders.
            if(other->Trigger || ::Distance(anchor->Coord,other->Coord)>5*ICON_LEPTON_W)continue;
            group.push_back(other);if(group.size()==10)break;
        }
        if(group.size()!=10)continue;
        std::int64_t ratios=0;
        for(auto member:group)ratios+=std::min<std::int64_t>(65536,std::int64_t(member->Strength)*65536/MAX(1,Campaign_Max_Health(member)));
        tiers[anchor->As_Target()]=Tier(anchor)+1;
        anchor->Strength=MAX(1,int(std::int64_t(Campaign_Max_Health(anchor))*ratios/(10*65536)));
        anchor->Mark(MARK_CHANGE);
        // Native deletion releases occupancy, team/radio references and heap counts without combat deaths or crew.
        for(unsigned i=1;i<group.size();++i) {
            auto member=group[i];TARGET id=member->As_Target();consumed.insert(id);
            member->Stun();Campaign_Enemies_Forget(member);delete member;
        }
        // A surviving anchor may only evolve once per pass; future passes can fuse the next tier.
        consumed.insert(anchor->As_Target());
        Map.Flag_To_Redraw(true);
    }
}
bool Save(FileClass& file) {
    std::uint32_t count=tiers.size();
    if(file.Write(magic,8)!=8 || file.Write(&count,4)!=4)return false;
    for(auto const& e:tiers){std::uint32_t data[2]={std::uint32_t(e.first),e.second};if(file.Write(data,8)!=8)return false;}
    return true;
}
bool Load(FileClass& file) {
    Reset();char header[8];int n=file.Read(header,8);if(!n)return true;
    if(n!=8 || std::memcmp(header,magic,8))return false;
    std::uint32_t count;if(file.Read(&count,4)!=4 || count>UNIT_MAX+INFANTRY_MAX+AIRCRAFT_MAX)return false;
    for(unsigned i=0;i<count;++i){std::uint32_t data[2];
        if(file.Read(data,8)!=8 || !ValidID(data[0]) || !data[1] || data[1]>maxTier || tiers.count(data[0]))return false;
        // Parsing precedes Decode_All_Pointers: never dereference Class/House here.
        tiers[data[0]]=data[1];
    }
    return true;
}
bool Fixup() {
    for(auto e:tiers){auto object=As_Techno(e.first);if(!object || !Mobile(object) || !object->IsActive)return false;}
    return true;
}
}
