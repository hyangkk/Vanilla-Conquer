// Local, deterministic congestion recovery. Native movement still owns every step.
#include "function.h"
#include "siegetank.h"
#include "campaigntraffic.h"
#include <algorithm>
#include <map>
#include <queue>
#include <vector>
namespace {
struct Observation {COORDINATE position;TARGET goal;int changed;Observation(COORDINATE p=0,TARGET g=TARGET_NONE,int c=0):position(p),goal(g),changed(c){}};
std::map<TARGET,Observation> observations;
bool issuing=false;
bool Active(FootClass const* f)
{
    if(!f||!f->IsActive||f->IsInLimbo||f->Strength<=0||!f->House||f->Team||f->Trigger
        ||f->IsALoaner||f->IsUnloading||f->IsDeploying||f->IsTethered||f->In_Radio_Contact()
        ||!Map.In_Radar(Coord_Cell(f->Coord)))return false;
    if(f->What_Am_I()==RTTI_UNIT){auto u=static_cast<UnitClass const*>(f);
        if((SiegeTank::Is(u) && (u->Class->Type == UNIT_SIEGE_MODE || SiegeTank::Transitioning(u))) || u->Class->IsToHarvest||u->Class->IsTransporter||u->Class->Speed==SPEED_FLOAT
            ||u->Class->Speed==SPEED_HOVER||u->Class->Type==UNIT_MCV)return false;
    }else if(f->What_Am_I()!=RTTI_INFANTRY)return false;
    return true;
}
bool MovingMission(MissionType m)
{return m==MISSION_MOVE||m==MISSION_ATTACK||m==MISSION_HUNT||m==MISSION_GUARD_AREA;}
bool IdleMission(MissionType m){return m==MISSION_GUARD||m==MISSION_SLEEP||m==MISSION_STICKY;}
bool Yielding(FootClass const* f)
{
    // The unmodified game only overrides ATTACK, never MOVE. These native saved
    // fields uniquely identify our temporary sidestep, even after a saved-game load.
    return f->SuspendedMission!=MISSION_NONE && (MovingMission(f->SuspendedMission)||IdleMission(f->SuspendedMission))
        && (f->Mission==MISSION_MOVE||IdleMission(f->Mission));
}
FootClass* Blocker(FootClass* f,CELL cell)
{
    if(!Map.In_Radar(cell))return nullptr;
    for(auto o=Map[cell].Cell_Occupier();o;o=o->Next){
        if(o==f||!o->IsActive||o->IsInLimbo)continue;
        auto kind=o->What_Am_I();if(kind!=RTTI_UNIT&&kind!=RTTI_INFANTRY)continue;
        auto other=static_cast<FootClass*>(o);
        if(other->Strength>0&&other->House&&f->House->Is_Ally(other->House)&&other->House->Is_Ally(f->House))return other;
    }
    return nullptr;
}
FootClass* Ahead(FootClass* f)
{
    if(!Target_Legal(f->NavCom))return nullptr;
    CELL start=Coord_Cell(f->Coord),goal=As_Cell(f->NavCom);
    FacingType face=f->Path[0];if(face==FACING_NONE)face=Dir_Facing(::Direction(start,goal));
    if(auto b=Blocker(f,Adjacent_Cell(start,face)))return b;
    int dx=Cell_X(goal)-Cell_X(start),dy=Cell_Y(goal)-Cell_Y(start);
    FootClass* best=nullptr;int score=-1;
    for(FacingType dir=FACING_N;dir<FACING_COUNT;dir++){
        CELL c=Adjacent_Cell(start,dir);auto b=Blocker(f,c);if(!b)continue;
        int dot=(Cell_X(c)-Cell_X(start))*dx+(Cell_Y(c)-Cell_Y(start))*dy;
        if(dot>score&&dot>0){score=dot;best=b;}
    }
    return best;
}
CELL PassingPlace(FootClass* yield,FootClass* passing)
{
    CELL start=Coord_Cell(yield->Coord),origin=Coord_Cell(passing->Coord);
    CELL goal=Target_Legal(passing->NavCom)?As_Cell(passing->NavCom):start;
    int vx=Cell_X(goal)-Cell_X(origin),vy=Cell_Y(goal)-Cell_Y(origin);
    if(!vx&&!vy){vx=Cell_X(start)-Cell_X(origin);vy=Cell_Y(start)-Cell_Y(origin);}
    // Only a bounded local flood search, after observed two-second congestion.
    struct Node {CELL cell;int distance;};std::queue<Node> queue;queue.push({start,0});
    std::vector<CELL> seen{start};CELL best=0;int bestScore=-1000000;
    while(!queue.empty()){
        auto node=queue.front();queue.pop();CELL c=node.cell;
        int perp=ABS((Cell_X(c)-Cell_X(origin))*vy-(Cell_Y(c)-Cell_Y(origin))*vx);
        if(node.distance&&perp>0){
            int exits=0;for(FacingType face=FACING_N;face<FACING_COUNT;face++){
                CELL n=Adjacent_Cell(c,face);if(Map.In_Radar(n)&&yield->Can_Enter_Cell(n)==MOVE_OK)++exits;
            }
            // A parking bay must open out of the travel line; avoid replacing one
            // permanent corridor obstruction with another. Prefer near, open space.
            int score=exits*100-MIN(100,perp)*2-node.distance*12;
            if(exits>=3&&score>bestScore){bestScore=score;best=c;}
        }
        if(node.distance>=6)continue;
        for(FacingType face=FACING_N;face<FACING_COUNT;face++){
            CELL next=Adjacent_Cell(c,face);if(!Map.In_Radar(next)||std::find(seen.begin(),seen.end(),next)!=seen.end()
                ||yield->Can_Enter_Cell(next)!=MOVE_OK)continue;
            if(Cell_X(c)!=Cell_X(next)&&Cell_Y(c)!=Cell_Y(next)){
                if(yield->Can_Enter_Cell(XY_Cell(Cell_X(c),Cell_Y(next)))!=MOVE_OK
                    ||yield->Can_Enter_Cell(XY_Cell(Cell_X(next),Cell_Y(c)))!=MOVE_OK)continue;
            }
            seen.push_back(next);queue.push({next,node.distance+1});
        }
    }
    return best;
}
bool Yield(FootClass* f,FootClass* passing)
{
    if(!Active(f)||f->House!=passing->House||f->IsDriving||f->IsRotating||f->SuspendedMission!=MISSION_NONE
        ||(!MovingMission(f->Mission)&&!IdleMission(f->Mission)))return false;
    CELL cell=PassingPlace(f,passing);if(!cell)return false;
    issuing=true;f->Override_Mission(MISSION_MOVE,TARGET_NONE,::As_Target(cell));f->Commence();issuing=false;
    f->Path[0]=FACING_NONE;f->PathDelay=0;f->TryTryAgain=FootClass::PATH_RETRY;
    auto& state=observations[f->As_Target()];state={f->Coord,f->NavCom,Frame};
    return true;
}
void Recover(FootClass* f)
{
    if(!Active(f))return;
    auto id=f->As_Target();auto& state=observations[id];
    bool yield=Yielding(f);
    if(yield&&(!Target_Legal(f->NavCom)||f->Distance(f->NavCom)<0x0080)&&!f->IsDriving){
        issuing=true;f->Restore_Mission();f->Commence();issuing=false;
        f->Path[0]=FACING_NONE;f->PathDelay=0;state={f->Coord,f->NavCom,Frame};return;
    }
    if(!MovingMission(f->Mission)||!Target_Legal(f->NavCom)){
        state={f->Coord,f->NavCom,Frame};return;
    }
    if(Distance(state.position,f->Coord)>=0x0040||state.goal!=f->NavCom||Frame<state.changed){state={f->Coord,f->NavCom,Frame};return;}
    if(Frame-state.changed<2*TICKS_PER_SECOND||f->IsDriving||f->IsRotating)return;
    if(yield){
        // Never keep a failed sidestep forever. Restore the actual order and let
        // the next recovery select a different passing bay from its new position.
        if(Frame-state.changed>=8*TICKS_PER_SECOND){issuing=true;f->Restore_Mission();f->Commence();issuing=false;state={f->Coord,f->NavCom,Frame};}
        f->Path[0]=FACING_NONE;f->PathDelay=0;f->TryTryAgain=FootClass::PATH_RETRY;return;
    }
    // A retry cannot resurrect a cancelled player command because the current
    // native NavCom remains authoritative, rather than an old cached destination.
    f->Path[0]=FACING_NONE;f->PathDelay=0;f->TryTryAgain=FootClass::PATH_RETRY;
    auto b=Ahead(f);if(!b)return;
    if(!Active(b)||b->IsDriving||b->IsRotating)return;
    if(!Target_Legal(b->NavCom)){Yield(b,f);return;}
    CELL a=Coord_Cell(f->Coord),end=As_Cell(f->NavCom),bc=Coord_Cell(b->Coord),be=As_Cell(b->NavCom);
    int dot=(Cell_X(end)-Cell_X(a))*(Cell_X(be)-Cell_X(bc))+(Cell_Y(end)-Cell_Y(a))*(Cell_Y(be)-Cell_Y(bc));
    if(dot>0)return; // Same-direction columns queue; they do not jostle one another.
    // Stable IDs break head-on/crossing ties. Exactly one unit yields.
    if(f->As_Target()>b->As_Target()){if(!Yield(f,b))Yield(b,f);}
    else if(!Yield(b,f))Yield(f,b);
}
}
void Campaign_Traffic_Reset(){observations.clear();issuing=false;}
void Campaign_Traffic_Forget(TARGET id){observations.erase(id);}
void Campaign_Traffic_Order(FootClass* f,TARGET destination)
{
    if(issuing||!f||!Yielding(f))return;
    if((Target_Legal(destination)&&destination!=f->NavCom)
        ||f->Mission==MISSION_STOP||(f->MissionQueue!=MISSION_NONE&&f->MissionQueue!=MISSION_MOVE)){
        f->SuspendedMission=MISSION_NONE;f->SuspendedNavCom=TARGET_NONE;f->SuspendedTarCom=TARGET_NONE;
        observations.erase(f->As_Target());
    }
}
bool Campaign_Traffic_Wait(FootClass* f)
{
    if(GameToPlay!=GAME_NORMAL||!Active(f)||!MovingMission(f->Mission)||!Target_Legal(f->NavCom))return false;
    CELL center=Coord_Cell(f->Coord);
    // Retain intent only for a nearby friendly traffic obstruction. Permanent
    // terrain and enemy obstacles still use the native failure/attack behavior.
    for(int dy=-2;dy<=2;++dy)for(int dx=-2;dx<=2;++dx){
        int x=Cell_X(center)+dx,y=Cell_Y(center)+dy;if(x<0||x>=MAP_CELL_W||y<0||y>=MAP_CELL_H)continue;
        if(Blocker(f,XY_Cell(x,y))){f->TryTryAgain=FootClass::PATH_RETRY;f->PathDelay=TICKS_PER_SECOND;return true;}
    }
    return false;
}
void Campaign_Traffic_AI()
{
    if(GameToPlay!=GAME_NORMAL||!GameActive||ScenarioInit||!PlayerPtr||!PlayerPtr->IsHuman)return;
    std::vector<FootClass*> list;for(int i=0;i<Units.Count();++i)list.push_back(Units.Ptr(i));for(int i=0;i<Infantry.Count();++i)list.push_back(Infantry.Ptr(i));
    for(auto f:list)Recover(f);
}
