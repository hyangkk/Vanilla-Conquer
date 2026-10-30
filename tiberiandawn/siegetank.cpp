#include "function.h"
#include "siegetank.h"
#include <map>
#include <vector>
#include <cstdint>
#include <cstring>
namespace SiegeTank {
namespace {
std::map<TARGET, int> transitions;
const char magic[8] = {'C','N','C','S','G','E','0','1'};
const int transitionTicks = 3 * TICKS_PER_SECOND;
bool Ground(TechnoClass const* target) {
    return target && target->IsActive && !target->IsInLimbo && target->Strength > 0
        && (target->What_Am_I() != RTTI_AIRCRAFT || static_cast<AircraftClass const*>(target)->Altitude == 0);
}
}
bool Is(TechnoClass const* object) {
    if (!object || object->What_Am_I() != RTTI_UNIT) return false;
    auto type = static_cast<UnitClass const*>(object)->Class->Type;
    return type == UNIT_SIEGE_TANK || type == UNIT_SIEGE_MODE;
}
bool Transitioning(UnitClass const* unit) { return Is(unit) && transitions.count(unit->As_Target()); }
void Forget(UnitClass* unit) { if (unit) transitions.erase(unit->As_Target()); }
void Reset() { transitions.clear(); }
bool Toggle(UnitClass* unit) {
    if (!Is(unit) || !Ground(unit)) return false;
    if (Transitioning(unit)) return true;
    unit->Assign_Destination(TARGET_NONE);
    unit->Path[0] = FACING_NONE;
    // Finish the current native movement step; never strand occupation bits mid-cell.
    if (unit->IsDriving || unit->IsRotating) return false;
    unit->Assign_Target(TARGET_NONE);
    unit->Set_Speed(0);
    transitions[unit->As_Target()] = Frame + transitionTicks;
    unit->Arm = MAX(int(unit->Arm), transitionTicks);
    unit->Mark(MARK_CHANGE);
    return true;
}
void AI(UnitClass* unit) {
    if (!Is(unit) || !Ground(unit)) return;
    auto it = transitions.find(unit->As_Target());
    if (it != transitions.end()) {
        if (Frame < it->second) return;
        const_cast<UnitTypeClass const*&>(unit->Class) = &UnitTypeClass::As_Reference(*unit == UNIT_SIEGE_TANK ? UNIT_SIEGE_MODE : UNIT_SIEGE_TANK);
        transitions.erase(it);
        unit->Assign_Destination(TARGET_NONE);
        unit->Assign_Mission(MISSION_GUARD);
        unit->Commence();
        unit->Look(false);
        unit->Mark(MARK_CHANGE);
    }
    if (*unit != UNIT_SIEGE_MODE || Frame % 5) return;
    unit->Assign_Destination(TARGET_NONE);
    auto valid = [&](TechnoClass* target) {
        return Ground(target) && target != unit && target->House && target->Class_Of().IsLegalTarget
            && target->House != unit->House && !unit->House->Is_Ally(target->House)
            && !target->House->Is_Ally(unit->House) && target->House->Class->House != HOUSE_NEUTRAL
            && !target->Is_Cloaked(unit->House) && target->IsDiscoveredByPlayer
            && unit->Distance(target) >= 2 * ICON_LEPTON_W && unit->In_Range(target->As_Target(), 0);
    };
    // A nearby enemy inside minimum range must not monopolize target acquisition.
    if (valid(As_Techno(unit->TarCom))) return;
    TechnoClass* best = nullptr;
    int distance = 0x7fffffff;
    auto consider = [&](TechnoClass* target) {
        if (valid(target) && unit->Distance(target) < distance) { best = target; distance = unit->Distance(target); }
    };
    for (int i=0; i<Units.Count(); ++i) consider(Units.Ptr(i));
    for (int i=0; i<Infantry.Count(); ++i) consider(Infantry.Ptr(i));
    for (int i=0; i<Buildings.Count(); ++i) consider(Buildings.Ptr(i));
    unit->Assign_Target(best ? best->As_Target() : TARGET_NONE);
}
bool Save(FileClass& file) {
    std::uint32_t count = transitions.size();
    if (file.Write(magic,8)!=8 || file.Write(&count,sizeof(count))!=sizeof(count)) return false;
    for (auto const& entry : transitions) {
        std::uint32_t data[2] = {std::uint32_t(entry.first), std::uint32_t(Bound(entry.second-Frame,0,transitionTicks))};
        if (file.Write(data,sizeof(data))!=sizeof(data)) return false;
    }
    return true;
}
bool Load(FileClass& file) {
    Reset(); char header[8]; int n=file.Read(header,8);
    if (!n) return true; // Existing mission and user saves have no siege footer.
    if (n!=8 || std::memcmp(header,magic,8)) return false;
    std::uint32_t count;
    if (file.Read(&count,sizeof(count))!=sizeof(count) || count>UNIT_MAX) return false;
    for (unsigned i=0;i<count;++i) {
        std::uint32_t data[2];
        if (file.Read(data,sizeof(data))!=sizeof(data) || !Is_Target_Unit(data[0])
            || Target_Value(data[0]) >= UNIT_MAX || data[1]>transitionTicks || transitions.count(data[0])) return false;
        // Class pointers remain encoded until Decode_All_Pointers after all footers.
        transitions[data[0]]=Frame+data[1];
    }
    return true;
}
bool Impact(BulletClass* bullet, TARGET intended) {
    if (!bullet || (bullet->Class->Type!=BULLET_SIEGE_TANK && bullet->Class->Type!=BULLET_SIEGE_CANNON)) return false;
    bool splash = bullet->Class->Type==BULLET_SIEGE_CANNON;
    std::vector<TARGET> victims;
    if (!splash) { if (Ground(As_Techno(intended))) victims.push_back(intended); }
    else {
        auto consider=[&](TechnoClass* target) {
            if (Ground(target) && ::Distance(bullet->Coord,target->Target_Coord())<=ICON_LEPTON_W*5/4) victims.push_back(target->As_Target());
        };
        for(int i=0;i<Units.Count();++i) consider(Units.Ptr(i));
        for(int i=0;i<Infantry.Count();++i) consider(Infantry.Ptr(i));
        for(int i=0;i<Buildings.Count();++i) consider(Buildings.Ptr(i));
        for(int i=0;i<Aircraft.Count();++i) consider(Aircraft.Ptr(i));
    }
    // Snapshot IDs before applying damage: death may allocate/delete other objects.
    for(auto id:victims) {
        auto target=As_Techno(id); if(!Ground(target)) continue;
        int damage=bullet->Strength;
        if(splash) {
            int distance=::Distance(bullet->Coord,target->Target_Coord());
            if(distance>ICON_LEPTON_W*25/32) damage/=4;
            else if(distance>ICON_LEPTON_W*10/32) damage/=2;
        }
        if(target->What_Am_I()==RTTI_INFANTRY) damage/=2;
        else if(target->What_Am_I()==RTTI_UNIT) {
            UnitType type=*static_cast<UnitClass*>(target);
            if(type==UNIT_BIKE||type==UNIT_BUGGY||type==UNIT_JEEP) damage=damage*3/4;
        }
        if(damage) target->Take_Damage(damage,0,WARHEAD_SIEGE_EXPLOSIVE,bullet->Payback);
    }
    CELL cell=Coord_Cell(bullet->Coord);
    if(Map.In_Radar(cell) && Map[cell].Overlay!=OVERLAY_NONE && OverlayTypeClass::As_Reference(Map[cell].Overlay).IsWall) Map[cell].Reduce_Wall(bullet->Strength);
    return true;
}
}
