//
// Copyright 2020 Electronic Arts Inc.
//
// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
// software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.

// TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed
// in the hope that it will be useful, but with permitted additional restrictions
// under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT
// distributed with this program. You should have received a copy of the
// GNU General Public License along with permitted additional restrictions
// with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection

/* $Header:   F:\projects\c&c\vcs\code\combat.cpv   2.17   16 Oct 1995 16:48:32   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : COMBAT.CPP                                                   *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : September 19, 1994                                           *
 *                                                                                             *
 *                  Last Update : January 1, 1995 [JLB]                                        *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Explosion_Damage -- Inflict an explosion damage affect.                                   *
 *   Modify_Damage -- Adjusts damage to reflect the nature of the target.                      *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "function.h"
#include "campaigndefense.h"
#include <vector>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <map>

int Modify_Damage(int damage, WarheadType warhead, ArmorType armor);
void Explosion_Damage(COORDINATE coord, unsigned strength, TechnoClass* source, WarheadType warhead);

/***********************************************************************************************
 * Modify_Damage -- Adjusts damage to reflect the nature of the target.                        *
 *                                                                                             *
 *    This routine is the core of combat tactics. It implements the                            *
 *    affect various armor types have against various weapon types. By                         *
 *    careful exploitation of this table, tactical advantage can be                            *
 *    obtained.                                                                                *
 *                                                                                             *
 * INPUT:   damage   -- The damage points to process.                                          *
 *                                                                                             *
 *          warhead  -- The source of the damage points.                                       *
 *                                                                                             *
 *          armor    -- The type of armor defending against the damage.                        *
 *                                                                                             *
 *          distance -- The distance (in leptons) from the source of the damage.               *
 *                                                                                             *
 * OUTPUT:  Returns with the adjusted damage points to inflict upon the                        *
 *          target.                                                                            *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   04/16/1994 JLB : Created.                                                                 *
 *   04/17/1994 JLB : Always does a minimum of damage.                                         *
 *   01/01/1995 JLB : Takes into account distance from damage source.                          *
 *=============================================================================================*/
int Modify_Damage(int damage, WarheadType warhead, ArmorType armor, int distance)
{
    /*
    **	If there is no raw damage value to start with, then
    **	there can be no modified damage either.
    */
    if (Special.IsInert || !damage || warhead == WARHEAD_NONE)
        return (0);

    WarheadTypeClass const* whead = &Warheads[warhead];

    damage = Fixed_To_Cardinal(damage, whead->Modifier[armor]);

    /*
    **	Reduce damage according to the distance from the impact point.
    */
    if (damage) {
        //		if (distance < 0x0010) damage *= 2;			// Double damage for direct hits.
        distance >>= whead->SpreadFactor;
        distance = Bound(distance, 0, 16);
        damage >>= distance;
    }

    /*
    **	If damage was indicated, then it should never drop below one damage point regardless
    **	of modifiers. This allows a very weak attacker to eventually destroy anything it
    **	fires upon, given enough time.
    */
    return (damage);
}

/***********************************************************************************************
 * Explosion_Damage -- Inflict an explosion damage affect.                                     *
 *                                                                                             *
 *    Processes the collateral damage affects typically caused by an                           *
 *    explosion.                                                                               *
 *                                                                                             *
 * INPUT:   coord    -- The coordinate of ground zero.                                         *
 *                                                                                             *
 *          strength -- Raw damage points at ground zero.                                      *
 *                                                                                             *
 *          source   -- Source of the explosion (who is responsible).                          *
 *                                                                                             *
 *          warhead  -- The kind of explosion to process.                                      *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   This routine can consume some time and will affect the AI                       *
 *             of nearby enemy units (possibly).                                               *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/16/1991 JLB : Created.                                                                 *
 *   11/30/1991 JLB : Uses coordinate system.                                                  *
 *   12/27/1991 JLB : Radius of explosion damage effect.                                       *
 *   04/13/1994 JLB : Streamlined.                                                             *
 *   04/16/1994 JLB : Warhead damage type modifier.                                            *
 *   04/17/1994 JLB : Cleaned up.                                                              *
 *   06/20/1994 JLB : Uses object pointers to distribute damage.                               *
 *   06/20/1994 JLB : Source is a pointer.                                                     *
 *=============================================================================================*/
void Explosion_Damage(COORDINATE coord, unsigned strength, TechnoClass* source, WarheadType warhead)
{
    CELL cell;                // Cell number under explosion.
    ObjectClass* object;      // Working object pointer.
    ObjectClass* objects[32]; // Maximum number of objects that can be damaged.
    int distance;             // Distance to unit.
    int range;                // Damage effect radius.
    int index;
    int count; // Number of vehicle IDs in list.

    if (!strength || Special.IsInert || warhead == WARHEAD_NONE)
        return;

    WarheadTypeClass const* whead = &Warheads[warhead];
    range = ICON_LEPTON_W + (ICON_LEPTON_W >> 1);
    cell = Coord_Cell(coord);
    if ((unsigned)cell >= MAP_CELL_TOTAL)
        return;
    //	if (!Map.In_Radar(cell)) return;

    CellClass* cellptr = &Map[cell];
    ObjectClass* impacto = cellptr->Cell_Occupier();

    /*
    **	Fill the list of unit IDs that will have damage
    **	assessed upon them. The units can be lifted from
    **	the cell data directly.
    */
    count = 0;
    for (FacingType i = FACING_NONE; i < FACING_COUNT; i++) {
        /*
        **	Fetch a pointer to the cell to examine. This is either
        **	an adjacent cell or the center cell. Damage never spills
        **	further than one cell away.
        */
        if (i != FACING_NONE) {
            cellptr = Map[cell].Adjacent_Cell(i);
            if (!cellptr)
                continue;
        }

        /*
        **	Add all objects in this cell to the list of objects to possibly apply
        ** damage to. The list stops building when the object pointer list becomes
        ** full.  Do not include overlapping objects; selection state can affect
        ** the overlappers, and this causes multiplayer games to go out of sync.
        */
        object = cellptr->Cell_Occupier();
        while (object) {
            if (!object->IsToDamage && object != source) {
                object->IsToDamage = true;
                objects[count++] = object;
                if (count >= (sizeof(objects) / sizeof(objects[0])))
                    break;
            }
            object = object->Next;
        }
        if (count >= (sizeof(objects) / sizeof(objects[0])))
            break;
    }

    /*
    **	Sweep through the units to be damaged and damage them. When damaging
    **	buildings, consider a hit on any cell the building occupies as if it
    **	were a direct hit on the building's center.
    */
    for (index = 0; index < count; index++) {
        object = objects[index];

        object->IsToDamage = false;
        if (object->What_Am_I() == RTTI_BUILDING && impacto == object) {
            distance = 0;
        } else {
            distance = Distance(coord, object->Center_Coord());
        }
        if (object->IsDown && !object->IsInLimbo && distance < range) {
            int damage = strength;

            /*
            **	High explosive does double damage against aircraft.
            */
            if (warhead == WARHEAD_HE && object->What_Am_I() == RTTI_AIRCRAFT) {
                damage *= 2;
            }

            /*
            **	Apply the damage to the object.
            */
            if (damage) {
                object->Take_Damage(damage, distance, warhead, source);
            }
        }
    }

    /*
    **	If there is a wall present at this location, it may be destroyed. Check to
    **	make sure that the warhead is of the kind that can destroy walls.
    */
    cellptr = &Map[cell];
    cellptr->Reduce_Tiberium(strength / 10);
    if (cellptr->Overlay != OVERLAY_NONE) {
        OverlayTypeClass const* optr = &OverlayTypeClass::As_Reference(cellptr->Overlay);

        if (optr->IsWall) {
            if (whead->IsWallDestroyer || (whead->IsWoodDestroyer && optr->IsWooden)) {
                Map[cell].Reduce_Wall(strength);
            }
        }
    }
}


namespace {
std::array<unsigned char, BUILDING_MAX> samFractions{};
std::array<unsigned char, BUILDING_MAX> samHealthBoost{};
const char samMagic[8]={'C','N','C','S','A','M','0','3'};
}
void Campaign_SAM_Volley_Reset() { samFractions.fill(0); }
void Campaign_SAM_Volley_Forget(BuildingClass* sam)
{
    int id=Buildings.ID(sam);
    if (id>=0 && id<BUILDING_MAX) {samFractions[id]=0;samHealthBoost[id]=0;}
}
bool Campaign_SAM_Volley_Save(FileClass& file)
{
    return file.Write(samMagic,sizeof(samMagic))==sizeof(samMagic)
        && file.Write(samFractions.data(),samFractions.size())==int(samFractions.size())
        && file.Write(samHealthBoost.data(),samHealthBoost.size())==int(samHealthBoost.size())
        && Campaign_SAM_Chain_Save(file);
}
bool Campaign_SAM_Volley_Load(FileClass& file)
{
    Campaign_SAM_Volley_Reset();
    Campaign_SAM_Health_Reset();
    Campaign_SAM_Chain_Reset();
    char header[8];int n=file.Read(header,sizeof(header));
    if (!n) return true; // Existing saves have no fractional-volley extension.
    const char oldMagic[8]={'C','N','C','S','A','M','0','1'};
    const char v2Magic[8]={'C','N','C','S','A','M','0','2'};
    bool v2=n==sizeof(header)&&!std::memcmp(header,v2Magic,sizeof(header));
    bool legacy=n==sizeof(header)&&!std::memcmp(header,oldMagic,sizeof(header));
    if (n!=sizeof(header)||(!legacy&&!v2&&std::memcmp(header,samMagic,sizeof(header)))) return false;
    decltype(samFractions) data{};
    if (file.Read(data.data(),data.size())!=int(data.size())) return false;
    for (auto& value:data) {
        if (value >= (legacy?5:10)) return false;
        if (legacy) value*=2; // Preserve old fifths exactly as tenths.
    }
    decltype(samHealthBoost) health{};
    if (!legacy && !v2) {
        if (file.Read(health.data(),health.size())!=int(health.size())) return false;
        for (auto value:health) if (value>1) return false;
    }
    // Consume legacy density credits for compatibility, but never restore them.
    samHealthBoost=health;return Campaign_SAM_Chain_Load(file);
}

void Campaign_SAM_Health_Reset() { samHealthBoost.fill(0); }
void Campaign_SAM_Health_Apply(BuildingClass* sam)
{
    if (!sam || sam->Class->Type!=STRUCT_SAM) return;
    int id=Buildings.ID(sam);
    if (id<0 || id>=BUILDING_MAX) return;
    bool desired=Campaign_SAM_Ground(sam);
    if (desired!=bool(samHealthBoost[id])) {
        // Scale current HP along with the maximum: existing damage is preserved
        // proportionally and repeated save/load never multiplies HP again.
        if (desired) sam->Strength=MIN(int(sam->Strength)*5,int(sam->Class->MaxStrength)*5);
        else if (sam->Strength>0) sam->Strength=MAX(1,(sam->Strength+4)/5);
        samHealthBoost[id]=desired;
        sam->LastStrength=sam->Strength;
    }
}
void Campaign_SAM_Health_Apply_All()
{
    for (int i=0;i<Buildings.Count();++i) Campaign_SAM_Health_Apply(Buildings.Ptr(i));
}

namespace {
constexpr unsigned samBounceLimit=10;
struct SAMChain {
    unsigned remaining=samBounceLimit,rawDamage=0,count=0;
    HousesType owner=HOUSE_NONE;
    // Count every attempt; TARGET_NONE records a detonation after target loss.
    std::array<TARGET,samBounceLimit+1> visited{};
};
std::map<TARGET,SAMChain> samChains;
const char chainMagic[8]={'C','N','C','C','H','N','0','3'};
const char branchChainMagic[8]={'C','N','C','C','H','N','0','2'};
const char oldChainMagic[8]={'C','N','C','C','H','N','0','1'};
std::vector<TARGET> oldChainPower;
void Slow_SAM_Flight(BulletClass* bullet)
{
    int speed=bullet->Get_Speed();
    int desired=MAX(1,int(Fixed_To_Cardinal((int)bullet->Class->MaxSpeed,255))/2);
    if (speed<=0 || speed==desired) return;
    // Scale the remaining lifetime once; preserve the fuse target and arming state.
    bullet->Timer=MIN(255,(int(bullet->Timer)*speed+desired-1)/desired);
    bullet->Fly_Speed(256,(MPHType)desired);
}
}
void Campaign_SAM_Chain_Reset() { samChains.clear();oldChainPower.clear(); }
void Campaign_SAM_Chain_Forget(BulletClass* bullet)
{
    if (bullet) samChains.erase(Build_Target(KIND_BULLET,Bullets.ID(bullet)));
}
void Campaign_SAM_Chain_Start(BulletClass* bullet)
{
    if (!bullet || *bullet!=BULLET_SAM || !Campaign_SAM_Ground(bullet->Payback)) return;
    SAMChain chain;chain.rawDamage=bullet->Strength;chain.owner=bullet->Payback->House->Class->House;
    samChains[bullet->As_Target()]=chain;
    Slow_SAM_Flight(bullet);
}
void Campaign_SAM_Chain_Fixup()
{
    // Old saves have no chain footer; initialize their existing missiles once,
    // before the anti-air impact modifier can alter the stored projectile power.
    for (int i=0;i<Bullets.Count();++i) {
        auto bullet=Bullets.Ptr(i);
        auto id=bullet->As_Target();
        if (!samChains.count(id)) {
            if (Campaign_SAM_Ground(bullet->Payback))
                bullet->Strength=MIN((int)Weapons[WEAPON_NIKE].Attack*bullet->Payback->House->FirepowerBias,32767);
            Campaign_SAM_Chain_Start(bullet);
        }
        else if (std::find(oldChainPower.begin(),oldChainPower.end(),id)!=oldChainPower.end())
            bullet->Strength=samChains[id].rawDamage;
        if (samChains.count(id)) Slow_SAM_Flight(bullet);
    }
    oldChainPower.clear();
}
bool Campaign_SAM_Chain_Impact(BulletClass* bullet,TechnoClass* target)
{
    auto it=samChains.find(bullet->As_Target());
    if (it==samChains.end()) return false;
    SAMChain chain=it->second;
    if (GameToPlay!=GAME_NORMAL || *bullet!=BULLET_SAM) return false;
    // A miss still consumes a hop and continues from the missile's position.
    // Prefer targets not yet attempted by this missile; reuse if none remain.
    TARGET attempted=(target && target->IsActive)?target->As_Target():TARGET_NONE;
    bool hit=target && target->IsActive && !target->IsInLimbo && target->Strength>0
        && bullet->Distance(target)<ICON_LEPTON_W/2;
    COORDINATE origin=hit?target->Target_Coord():bullet->Coord;
    if (chain.count<chain.visited.size()) chain.visited[chain.count++]=attempted;
    if (hit) {
        int damage=bullet->Strength;
        target->Take_Damage(damage,0,bullet->Class->Warhead,bullet->Payback);
    }
    if (!chain.remaining) return true;
    auto house=HouseClass::As_Pointer(chain.owner);
    if (!house) return true;
    std::vector<TechnoClass*> candidates,previousTargets;
    auto consider=[&](TechnoClass* unit) {
        if (!unit || !unit->IsActive || unit->IsInLimbo || unit->Strength<=0 || !unit->House
            || unit->House==house || unit->House->Class->House==HOUSE_NEUTRAL
            || house->Is_Ally(unit->House) || unit->House->Is_Ally(house)
            || !unit->Techno_Type_Class()->IsLegalTarget || unit->Is_Cloaked(house)
            || (!unit->IsDiscoveredByPlayer && unit->What_Am_I()!=RTTI_AIRCRAFT)
            || !Map.In_Radar(Coord_Cell(unit->Coord))) return;
        int distance=Distance(origin,unit->Target_Coord());
        if (distance<=10*ICON_LEPTON_W) {
            bool previous=std::find(chain.visited.begin(),chain.visited.begin()+chain.count,unit->As_Target())!=chain.visited.begin()+chain.count;
            (previous?previousTargets:candidates).push_back(unit);
        }
    };
    for (int i=0;i<Units.Count();++i) consider(Units.Ptr(i));
    for (int i=0;i<Infantry.Count();++i) consider(Infantry.Ptr(i));
    for (int i=0;i<Aircraft.Count();++i) consider(Aircraft.Ptr(i));
    for (int i=0;i<Buildings.Count();++i) consider(Buildings.Ptr(i));
    if (candidates.empty()) {
        candidates.swap(previousTargets);
        // When revisiting, bounce away from the current target if another exists.
        if (candidates.size()>1) candidates.erase(std::remove_if(candidates.begin(),candidates.end(),
            [attempted](TechnoClass* unit){return unit->As_Target()==attempted;}),candidates.end());
    }
    if (candidates.empty()) return true;
    std::stable_sort(candidates.begin(),candidates.end(),[origin](TechnoClass* a,TechnoClass* b) {
        return Distance(origin,a->Target_Coord())<Distance(origin,b->Target_Coord());
    });
    auto next=candidates.front();
    auto bounce=new BulletClass(BULLET_SAM);
    if (!bounce) return true;
    bounce->Assign_Target(next->As_Target());
    bounce->Strength=chain.rawDamage;
    // Keep Payback unset during Unlimbo: infantry scatter must be measured
    // from this impact rather than clamped to the original SAM's range.
    if (!bounce->Unlimbo(origin,::Direction(origin,next->Target_Coord()))) {delete bounce;return true;}
    bounce->Payback=bullet->Payback;
    Slow_SAM_Flight(bounce);
    --chain.remaining;samChains[bounce->As_Target()]=chain;
    return true;
}
bool Campaign_SAM_Chain_Save(FileClass& file)
{
    std::uint32_t count=samChains.size();
    if (file.Write(chainMagic,8)!=8 || file.Write(&count,sizeof(count))!=sizeof(count)) return false;
    for (auto const& entry:samChains) {
        auto const& c=entry.second;
        std::uint32_t data[5+samBounceLimit+1]={std::uint32_t(entry.first),c.remaining,c.rawDamage,std::uint32_t(c.owner),c.count};
        for (unsigned j=0;j<c.visited.size();++j) data[5+j]=c.visited[j];
        if (file.Write(data,sizeof(data))!=sizeof(data)) return false;
    }
    return true;
}
bool Campaign_SAM_Chain_Load(FileClass& file)
{
    Campaign_SAM_Chain_Reset();char header[8];int n=file.Read(header,8);
    if (!n) return true;
    bool legacyPower=n==8 && !std::memcmp(header,oldChainMagic,8);
    bool legacy=legacyPower || (n==8 && !std::memcmp(header,branchChainMagic,8));
    if (n!=8 || (!legacy && std::memcmp(header,chainMagic,8))) return false;
    std::uint32_t count;
    if (file.Read(&count,sizeof(count))!=sizeof(count)||count>BULLET_MAX) return false;
    for (unsigned i=0;i<count;++i) {
        std::uint32_t data[5+samBounceLimit+1]{};
        unsigned words=legacy?8:5+samBounceLimit+1,history=words-5;
        unsigned bytes=words*sizeof(data[0]),limit=legacy?2:samBounceLimit;
        if (file.Read(data,bytes)!=bytes) return false;
        if (Target_Kind(data[0])!=KIND_BULLET||Target_Value(data[0])>=BULLET_MAX||samChains.count(data[0])
            || data[1]>limit||data[2]>32767||data[3]>=HOUSE_COUNT||data[4]>limit||data[1]+data[4]!=limit) return false;
        SAMChain chain;chain.remaining=legacy?samBounceLimit-data[4]:data[1];chain.rawDamage=data[2];chain.owner=HousesType(data[3]);chain.count=data[4];
        for (unsigned j=0;j<history;++j) {chain.visited[j]=data[5+j];if (j<chain.count && data[5+j]!=TARGET_NONE && !Is_Target_Unit(data[5+j]) && !Is_Target_Infantry(data[5+j]) && !Is_Target_Aircraft(data[5+j]) && !Is_Target_Building(data[5+j])) return false;}
        if (legacyPower) {
            chain.rawDamage=MIN(chain.rawDamage*2,32767u);
            oldChainPower.push_back(data[0]);
        }
        samChains[data[0]]=chain;
    }
    return true;
}
