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
int Interval(std::uint32_t round)
{
    return (90 - 5 * std::min(round,std::uint32_t(12))) * TICKS_PER_SECOND;
}
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
void PressAttack(FootClass* object,TechnoClass* base,int quarters)
{
    if (!object || !object->IsActive || object->IsInLimbo || object->Strength<=0
        || !Hostile(object->House) || object->Team || object->Trigger || object->IsALoaner || object->In_Radio_Contact()
        || !Map.In_Radar(Coord_Cell(object->Coord)) || Target_Value(object->As_Target())%4>=quarters) return;
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
    int seconds=wave>=8?2:wave>=4?5:10;
    if (Frame%(seconds*TICKS_PER_SECOND)) return;
    auto target=PlayerAnchor();if (!target) return;
    int quarters=1+std::min(wave/std::uint32_t(3),std::uint32_t(3));
    for (int i=0;i<Units.Count();++i) PressAttack(Units.Ptr(i),target,quarters);
    for (int i=0;i<Infantry.Count();++i) PressAttack(Infantry.Ptr(i),target,quarters);
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
        int side=(remaining?wave-1:wave)%4;
        int sides[4]={y-Map.MapCellY,Map.MapCellX+Map.MapCellWidth-1-x,
                      Map.MapCellY+Map.MapCellHeight-1-y,x-Map.MapCellX};
        int score=(edge<=1?100000:0)+(sides[side]<=1?20000:0)+distance;
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
    FootClass* object=nullptr;
    bool armored=dinosaur<0 && delivered%(wave>=6?2:wave>=3?3:4)==(wave>=6?1:wave>=3?2:3);
    bool placed=false;
    for (int attempt=0;attempt<(armored?2:1) && !placed;++attempt) {
        if (dinosaur>=0) object=new UnitClass(UnitType(dinosaur),faction);
        else if (armored && !attempt) object=new UnitClass(house->ActLike==HOUSE_BAD?UNIT_LTANK:UNIT_MTANK,faction);
        else object=new InfantryClass(armored || delivered%4==2?(house->ActLike==HOUSE_BAD?INFANTRY_E3:INFANTRY_E2):INFANTRY_E1,faction);
        if (!object) continue;
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
        // Narrow infantry-only approaches must not block the whole wave behind a tank.
        if (!placed) {delete object;object=nullptr;}
    }
    if (!placed) return false;
    object->Assign_Mission(MISSION_HUNT);object->Commence();
    object->Assign_Target(target->As_Target());object->Assign_Destination(target->As_Target());
    Campaign_Enemies_Mark_Extra(object); // The wave size already is the final count.
    if (!delivered) {
        char message[100];std::snprintf(message,sizeof(message),"ENEMY ASSAULT: WAVE %u (%u UNITS)",wave,remaining);
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
    Mobilize();
    if (nextFrame<0) {nextFrame=Frame+Interval(wave);return;}
    // Tighten old saved deadlines, preserving a sooner or partially delivered wave.
    nextFrame=MIN(nextFrame,Frame+Interval(wave));
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
    if (!Hostile(house) || !Present(house)) {remaining=0;nextFrame=Frame+Interval(wave);return;}
    if (!remaining) {
        entry=candidate;faction=house->Class->House;dinosaur=DinosaurType(house);
        ++wave;remaining=std::min(std::uint64_t(UNIT_MAX+INFANTRY_MAX),6+6*std::uint64_t(wave));delivered=0;
        nextFrame=Frame+Interval(wave);
    } else if (!entry || !arrivalLand[entry]) entry=candidate;
    bool any=false;
    for (int budget=0;budget<24 && remaining;++budget) {
        if (!Spawn(house,target)) break;
        any=true;
    }
    if (!remaining && Frame>=nextFrame) nextFrame=Frame+Interval(wave);
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
