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
#include <array>
#include <cstdint>
#include <cstring>

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

// The SAM perk is selected by the actual missile impact, never by shared warhead
// data. Ground bursts remain ground-only; airborne bursts affect flying aircraft.
void Campaign_SAM_Explosion_Damage(COORDINATE coord, unsigned strength, TechnoClass* source,
                                   WarheadType warhead, bool airborne)
{
    if (!strength || Special.IsInert || warhead == WARHEAD_NONE || !Campaign_SAM_Ground(source)) return;
    const int radius = 5 * (ICON_LEPTON_W + (ICON_LEPTON_W >> 1));
    CELL center = Coord_Cell(coord);
    if ((unsigned)center >= MAP_CELL_TOTAL) return;
    ObjectClass* impact = Map[center].Cell_Occupier();
    std::vector<TARGET> targets;
    auto collect = [&](ObjectClass* object) {
        if (object && object != source && !object->IsInLimbo && object->IsDown && !object->IsToDamage) {
            object->IsToDamage = true;
            targets.push_back(object->As_Target());
        }
    };
    if (airborne) {
        for (int i=0;i<Aircraft.Count();++i) {
            auto air=Aircraft.Ptr(i);
            if (air->Altitude>0) collect(air);
        }
    } else {
        const int cells=(radius+ICON_LEPTON_W-1)/ICON_LEPTON_W;
        for (int dy=-cells;dy<=cells;++dy) for (int dx=-cells;dx<=cells;++dx) {
            int x=Cell_X(center)+dx,y=Cell_Y(center)+dy;
            if (x<0||x>=MAP_CELL_W||y<0||y>=MAP_CELL_H) continue;
            for (auto object=Map[XY_Cell(x,y)].Cell_Occupier();object;object=object->Next) collect(object);
        }
    }
    for (auto target:targets) {
        auto object=As_Object(target);
        // A chain explosion may already have removed a collected target. New
        // objects reusing its heap slot do not carry the pending-damage marker.
        if (!object || !object->IsToDamage) continue;
        object->IsToDamage=false;
        int distance=object==impact && object->What_Am_I()==RTTI_BUILDING ? 0 : Distance(coord,object->Center_Coord());
        if (!object->IsInLimbo && object->IsDown && distance<radius) {
            int damage=strength;
            object->Take_Damage(damage,distance/5,warhead,source);
        }
    }
    if (!airborne) {
        const int cells=(radius+ICON_LEPTON_W-1)/ICON_LEPTON_W;
        auto whead=&Warheads[warhead];
        for (int dy=-cells;dy<=cells;++dy) for (int dx=-cells;dx<=cells;++dx) {
            int x=Cell_X(center)+dx,y=Cell_Y(center)+dy;
            if (x<0||x>=MAP_CELL_W||y<0||y>=MAP_CELL_H) continue;
            CELL cell=XY_Cell(x,y);int distance=Distance(coord,Cell_Coord(cell));
            if (distance>=radius) continue;
            unsigned damage=strength>>Bound((distance/5)>>whead->SpreadFactor,0,16);
            Map[cell].Reduce_Tiberium(damage/10);
            if (Map[cell].Overlay!=OVERLAY_NONE) {
                auto overlay=&OverlayTypeClass::As_Reference(Map[cell].Overlay);
                if (overlay->IsWall && (whead->IsWallDestroyer || (whead->IsWoodDestroyer&&overlay->IsWooden))) Map[cell].Reduce_Wall(damage);
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
void Campaign_SAM_Volley(BuildingClass* sam,TARGET target,int weapon)
{
    if (!Campaign_SAM_Ground(sam)) return;
    int id=Buildings.ID(sam);
    if (id<0 || id>=BUILDING_MAX) return;
    int enemies=0;
    std::vector<TARGET> alternatives;
    auto count=[&](FootClass* unit) {
        if (unit && unit->IsActive && !unit->IsInLimbo && unit->Strength>0
            && unit->House && unit->House!=sam->House && unit->House->Class->House!=HOUSE_NEUTRAL
            && !unit->House->Is_Ally(sam->House) && !sam->House->Is_Ally(unit->House)
            && Map.In_Radar(Coord_Cell(unit->Coord))) {
            if (Distance(sam->Center_Coord(),unit->Center_Coord())<=sam->Weapon_Range(weapon)/2) ++enemies;
            int value=0;
            if (unit->As_Target()!=target && sam->Evaluate_Object(THREAT_NORMAL,1<<unit->What_Am_I(),0,unit,value))
                alternatives.push_back(unit->As_Target());
        }
    };
    for (int i=0;i<Units.Count();++i) count(Units.Ptr(i));
    for (int i=0;i<Infantry.Count();++i) count(Infantry.Ptr(i));
    for (int i=0;i<Aircraft.Count();++i) count(Aircraft.Ptr(i));
    for (int i=0;i<Buildings.Count();++i) {
        auto building=Buildings.Ptr(i);int value=0;
        if (building!=sam && building->IsActive && !building->IsInLimbo && building->Strength>0
            && building->House && building->House!=sam->House && building->House->Class->House!=HOUSE_NEUTRAL
            && !sam->House->Is_Ally(building->House) && !building->House->Is_Ally(sam->House)
            && building->As_Target()!=target && sam->Evaluate_Object(THREAT_NORMAL,1<<RTTI_BUILDING,0,building,value))
            alternatives.push_back(building->As_Target());
    }
    int credit=enemies*5+samFractions[id],extra=credit/10;
    samFractions[id]=credit%10;
    unsigned char arm=sam->Arm;bool second=sam->IsSecondShot;
    // Qualified base calls create real missiles without recursively triggering a
    // new volley. Restore the normal salvo's rearm/shot phase afterwards.
    for (int i=0;i<extra && i<int(alternatives.size());++i) {
        if (!sam->TechnoClass::Fire_At(alternatives[i],weapon)) break;
    }
    sam->Arm=arm;sam->IsSecondShot=second;
}
bool Campaign_SAM_Volley_Save(FileClass& file)
{
    return file.Write(samMagic,sizeof(samMagic))==sizeof(samMagic)
        && file.Write(samFractions.data(),samFractions.size())==int(samFractions.size())
        && file.Write(samHealthBoost.data(),samHealthBoost.size())==int(samHealthBoost.size());
}
bool Campaign_SAM_Volley_Load(FileClass& file)
{
    Campaign_SAM_Volley_Reset();
    Campaign_SAM_Health_Reset();
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
    samFractions=data;samHealthBoost=health;return true;
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
