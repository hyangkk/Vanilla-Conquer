// Growing campaign reinforcements. State is appended after the optional enemy footer.
#include "function.h"
#include "campaignwaves.h"
#include "campaignenemies.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>
namespace {
const int interval = 3 * TICKS_PER_MINUTE;
const char magic[8] = {'C','N','C','W','A','V','0','1'};
int nextFrame = -1;
std::uint32_t wave = 0, remaining = 0, delivered = 0;
HousesType faction = HOUSE_NONE;
CELL entry = 0;
int dinosaur = -1;
std::vector<unsigned char> arrivalLand;
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
bool Present(HouseClass* house)
{
    for (int i=0;i<Buildings.Count();++i) {
        auto b=Buildings.Ptr(i);if (!b->IsInLimbo && b->Strength>0 && b->House==house) return true;
    }
    for (int i=0;i<Units.Count();++i) {
        auto u=Units.Ptr(i);if (!u->IsInLimbo && u->Strength>0 && u->House==house) return true;
    }
    for (int i=0;i<Infantry.Count();++i) {
        auto f=Infantry.Ptr(i);if (!f->IsInLimbo && f->Strength>0 && f->House==house) return true;
    }
    for (int i=0;i<Aircraft.Count();++i) {
        auto a=Aircraft.Ptr(i);if (!a->IsInLimbo && a->Strength>0 && a->House==house) return true;
    }
    return false;
}
HouseClass* EnemyHouse()
{
    // Check actual survivors as well as the campaign's defeat flag.
    for (int i=HOUSE_FIRST;i<HOUSE_COUNT;++i) {
        auto house=HouseClass::As_Pointer(HousesType(i));
        if (Hostile(house) && Present(house)) return house;
    }
    return nullptr;
}
bool Land(CELL cell)
{
    return Map.In_Radar(cell) && Ground[Map[cell].Land_Type()].Cost[SPEED_TRACK]
        && !Map[cell].Flag.Occupy.Building && Map[cell].Is_Clear_To_Move(true,true);
}
CELL EntryCell(CELL target)
{
    // Flood the player's reachable land, preferring its farthest map edge. On
    // islands, use that landmass's distant boundary rather than an unreachable coast.
    std::vector<unsigned char> seen(MAP_CELL_TOTAL,0);
    std::vector<CELL> queue;
    for (int radius=0;radius<=6 && queue.empty();++radius)
        for (int dy=-radius;dy<=radius;++dy) for (int dx=-radius;dx<=radius;++dx) {
            int x=Cell_X(target)+dx,y=Cell_Y(target)+dy;
            if (x<0||x>=MAP_CELL_W||y<0||y>=MAP_CELL_H) continue;
            CELL cell=XY_Cell(x,y);
            if (!seen[cell] && Land(cell)) {seen[cell]=1;queue.push_back(cell);}
        }
    int best=-1;CELL result=0;
    const int minimum=2;
    for (std::size_t i=0;i<queue.size();++i) {
        CELL cell=queue[i];int x=Cell_X(cell),y=Cell_Y(cell),distance=Distance(cell,target);
        int edge=MIN(MIN(x-Map.MapCellX,Map.MapCellX+Map.MapCellWidth-1-x),
                     MIN(y-Map.MapCellY,Map.MapCellY+Map.MapCellHeight-1-y));
        int score=(edge<=1?100000:0)+distance;
        if (distance>=minimum && !Map[cell].Flag.Composite && score>best) {best=score;result=cell;}
        for (int dy=-1;dy<=1;++dy) for (int dx=-1;dx<=1;++dx) {
            int nx=x+dx,ny=y+dy;
            if (nx<0||nx>=MAP_CELL_W||ny<0||ny>=MAP_CELL_H) continue;
            CELL next=XY_Cell(nx,ny);
            if (!seen[next] && Land(next)) {seen[next]=1;queue.push_back(next);}
        }
    }
    arrivalLand.swap(seen);
    return result;
}
int DinosaurType(HouseClass* house)
{
    for (int i=0;i<Units.Count();++i) {
        auto u=Units.Ptr(i);int type=u->Class->Type;
        if (!u->IsInLimbo && u->House==house && type>=UNIT_TRIC && type<=UNIT_STEG) return type;
    }
    return Special.IsJurassic ? UNIT_RAPT : -1;
}
bool Spawn(HouseClass* house,TechnoClass* target)
{
    FootClass* object;
    if (dinosaur>=0) object=new UnitClass(UnitType(dinosaur),faction);
    else if (delivered%4==3) object=new UnitClass(house->ActLike==HOUSE_BAD?UNIT_LTANK:UNIT_MTANK,faction);
    else object=new InfantryClass(delivered%4==2?(house->ActLike==HOUSE_BAD?INFANTRY_E3:INFANTRY_E2):INFANTRY_E1,faction);
    if (!object) return false;
    bool placed=false;
    for (int radius=0;radius<=6 && !placed;++radius)
        for (int dy=-radius;dy<=radius && !placed;++dy)
            for (int dx=-radius;dx<=radius;++dx) {
                if (MAX(ABS(dx),ABS(dy))!=radius) continue;
                int x=Cell_X(entry)+dx,y=Cell_Y(entry)+dy;
                if (x<0||x>=MAP_CELL_W||y<0||y>=MAP_CELL_H) continue;
                CELL cell=XY_Cell(x,y);
                if (!Map.In_Radar(cell)||!arrivalLand[cell]||object->Can_Enter_Cell(cell)!=MOVE_OK) continue;
                if (object->Unlimbo(Cell_Coord(cell),DIR_S)) {placed=true;break;}
            }
    if (!placed) {delete object;return false;}
    object->Assign_Mission(MISSION_HUNT);object->Commence();
    object->Assign_Target(target->As_Target());object->Assign_Destination(target->As_Target());
    Campaign_Enemies_Mark_Extra(object); // The wave size already is the final count.
    if (!delivered) {
        char message[100];std::snprintf(message,sizeof(message),"ENEMY REINFORCEMENTS: WAVE %u (%u UNITS)",wave,remaining);
        Messages.Add_Message(message,REMAP_RED,TPF_6PT_GRAD|TPF_USE_GRAD_PAL|TPF_FULLSHADOW,10*TICKS_PER_SECOND,0,0);
        Map.Flag_To_Redraw(true);
    }
    --remaining;++delivered;
    return true;
}
}
void Campaign_Waves_Reset()
{
    nextFrame=-1;wave=remaining=delivered=0;faction=HOUSE_NONE;entry=0;dinosaur=-1;arrivalLand.clear();
}
void Campaign_Waves_AI()
{
    if (GameToPlay!=GAME_NORMAL || !GameActive || ScenarioInit || !PlayerPtr || !PlayerPtr->IsHuman
        || PlayerWins || PlayerLoses || PlayerPtr->IsToWin || PlayerPtr->IsToLose || PlayerPtr->IsToDie) return;
    if (nextFrame<0) {nextFrame=Frame+interval;return;}
    if (Frame<nextFrame && !remaining) return;
    auto target=PlayerAnchor();if (!target) return;
    CELL candidate=EntryCell(Coord_Cell(target->Center_Coord()));
    if (!candidate) {
        for (int i=0;i<Units.Count() && !candidate;++i) {
            auto u=Units.Ptr(i);
            if (!u->IsInLimbo && u->Strength>0 && u->House==PlayerPtr && Map.In_Radar(Coord_Cell(u->Coord))) {
                target=u;candidate=EntryCell(Coord_Cell(u->Center_Coord()));
            }
        }
        for (int i=0;i<Infantry.Count() && !candidate;++i) {
            auto f=Infantry.Ptr(i);
            if (!f->IsInLimbo && f->Strength>0 && f->House==PlayerPtr && Map.In_Radar(Coord_Cell(f->Coord))) {
                target=f;candidate=EntryCell(Coord_Cell(f->Center_Coord()));
            }
        }
    }
    if (!candidate) return;
    auto house=remaining?HouseClass::As_Pointer(faction):EnemyHouse();
    if (!Hostile(house) || !Present(house)) {remaining=0;nextFrame=Frame+interval;return;}
    if (!remaining) {
        entry=candidate;faction=house->Class->House;dinosaur=DinosaurType(house);
        ++wave;remaining=std::min(std::uint64_t(UNIT_MAX+INFANTRY_MAX),4+2*std::uint64_t(wave));delivered=0;
        nextFrame=Frame+interval;
    } else if (!entry || !arrivalLand[entry]) entry=candidate;
    bool any=false;
    for (int budget=0;budget<8 && remaining;++budget) {
        if (!Spawn(house,target)) break;
        any=true;
    }
    if (!remaining && Frame>=nextFrame) nextFrame=Frame+interval;
    else if (remaining && !any) entry=candidate;
}
bool Campaign_Waves_Save(FileClass& file)
{
    std::int32_t data[7]={nextFrame,std::int32_t(wave),std::int32_t(remaining),std::int32_t(delivered),
        std::int32_t(faction),std::int32_t(entry),dinosaur};
    return file.Write(magic,sizeof(magic))==sizeof(magic) && file.Write(data,sizeof(data))==sizeof(data);
}
bool Campaign_Waves_Load(FileClass& file)
{
    char header[sizeof(magic)];int bytes=file.Read(header,sizeof(header));
    if (!bytes) return true; // The previous enemy multiplier footer ends here.
    if (bytes!=sizeof(header)||std::memcmp(header,magic,sizeof(magic))) return false;
    std::int32_t data[7];if (file.Read(data,sizeof(data))!=sizeof(data)) return false;
    if (data[0]<-1||data[1]<0||data[2]<0||data[2]>UNIT_MAX+INFANTRY_MAX||data[3]<0||data[3]>UNIT_MAX+INFANTRY_MAX
        ||data[2]+data[3]>UNIT_MAX+INFANTRY_MAX||data[4]<HOUSE_NONE||data[4]>=HOUSE_COUNT
        ||(data[2]>0&&(data[4]==HOUSE_NONE||data[5]==0))
        ||data[5]<0||data[5]>=MAP_CELL_TOTAL||(data[6]!=-1&&(data[6]<UNIT_TRIC||data[6]>UNIT_STEG))) return false;
    nextFrame=data[0];wave=data[1];remaining=data[2];delivered=data[3];faction=HousesType(data[4]);entry=CELL(data[5]);dinosaur=data[6];
    return true;
}
