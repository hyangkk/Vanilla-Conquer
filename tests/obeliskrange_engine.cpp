// Isolated verification harness: compiled in place of the offline save exporter only.
#include "function.h"
#include <cstdio>
#include <string>
static int failures = 0;
static void Check(const char* label, bool ok) {
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", label);
    std::fflush(stdout);
    if (!ok) ++failures;
}
bool Generate_Mission_Starts(const char* directory) {
    const std::string save = std::string(directory) + "/fixture.sav";
    GameToPlay = GAME_NORMAL;
    Check("load real GDI mission save", Load_Game(save.c_str()));
    if (!PlayerPtr) return false;
    HouseClass* good = HouseClass::As_Pointer(HOUSE_GOOD);
    HouseClass* bad = HouseClass::As_Pointer(HOUSE_BAD);
    good->IsHuman = true; bad->IsHuman = false; PlayerPtr = good;
    BuildingClass* mine = new BuildingClass(STRUCT_OBELISK, HOUSE_GOOD);
    BuildingClass* enemy = new BuildingClass(STRUCT_OBELISK, HOUSE_BAD);
    BuildingClass* tower = new BuildingClass(STRUCT_ATOWER, HOUSE_GOOD);
    if (!mine || !enemy || !tower) return false;
    mine->Coord = enemy->Coord = XY_Coord(20*256,20*256);
    mine->IsLocked = enemy->IsLocked = true;
    mine->IsCharged = enemy->IsCharged = true;
    mine->Arm = enemy->Arm = 0;
    Check("GDI owned Obelisk range = 3840 (15 cells)", mine->Weapon_Range(0)==3840);
    Check("enemy Obelisk range = 1920 (7.5 cells)", enemy->Weapon_Range(0)==1920);
    const COORDINATE fire = mine->Fire_Coord(0);
    for (int distance : {7*256, 8*256, 12*256, 15*256, 15*256+1}) {
        COORDINATE target = XY_Coord(Coord_X(fire)+distance,Coord_Y(fire));
        char label[128];
        std::snprintf(label,sizeof(label),"own coordinate range boundary at %d leptons",distance);
        Check(label,mine->In_Range(target,0)==(distance<=3840));
        std::snprintf(label,sizeof(label),"enemy coordinate range boundary at %d leptons",distance);
        Check(label,enemy->In_Range(target,0)==(distance<=1920));
    }
    TARGET far = As_Target(Coord_Cell(XY_Coord(Coord_X(fire)+12*256,Coord_Y(fire))));
    TARGET near = As_Target(Coord_Cell(XY_Coord(Coord_X(fire)+7*256,Coord_Y(fire))));
    Check("own charged Obelisk Can_Fire at 12 cells", mine->Can_Fire(far,0)==FIRE_OK);
    Check("enemy charged Obelisk cannot fire at 12 cells", enemy->Can_Fire(far,0)==FIRE_RANGE);
    Check("enemy can still fire at 7 cells", enemy->Can_Fire(near,0)==FIRE_OK);
    Check("advanced guard tower range unchanged", tower->Weapon_Range(0)==Weapons[tower->Class->Primary].Range);
    Check("laser damage still 200 and ROF still 90", Weapons[WEAPON_OBELISK_LASER].Attack==200 && Weapons[WEAPON_OBELISK_LASER].ROF==90);
    BulletClass* laser = static_cast<TechnoClass*>(mine)->Fire_At(far,0);
    Check("own Obelisk creates actual laser projectile at 12 cells", laser != nullptr);
    good->IsHuman=false;
    Check("AI-controlled player house gets no bonus",mine->Weapon_Range(0)==1920);
    good->IsHuman=true;
    GameToPlay=GAME_SKIRMISH;
    Check("skirmish gets no campaign bonus",mine->Weapon_Range(0)==1920);
    GameToPlay=GAME_NORMAL;
    good->IsHuman=false; bad->IsHuman=true; PlayerPtr=bad;
    Check("Nod owned Obelisk range = 3840",enemy->Weapon_Range(0)==3840);
    Check("opposing GDI Obelisk range = 1920",mine->Weapon_Range(0)==1920);
    mine->House=bad;
    Check("ownership transferred to player gains range immediately",mine->Weapon_Range(0)==3840);
    mine->House=good;
    Check("ownership transferred away loses range immediately",mine->Weapon_Range(0)==1920);
    std::printf("TOTAL FAILURES %d\n",failures);
    return failures==0;
}
