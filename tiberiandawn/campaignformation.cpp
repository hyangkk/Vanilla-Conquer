// Campaign squads gather in open ground, then approach on separate formation slots.
#include "function.h"
#include "campaignenemies.h"
#include "campaignformation.h"
#include <algorithm>
#include <map>
#include <set>
#include <queue>
#include <vector>
#include <cstdint>
#include <cstring>
#include <limits>
namespace {
struct Member{TARGET id;CELL slot;};
struct Squad{HousesType house;TARGET target;CELL rally;int deadline;unsigned phase;std::vector<Member> members;};
std::map<unsigned,Squad> squads;
std::map<TARGET,unsigned> membership;
unsigned nextID=1;
const char magic[8]={'C','N','C','F','R','M','0','1'};
bool MobileID(TARGET id){return (Is_Target_Unit(id)&&Target_Value(id)<UNIT_MAX)||(Is_Target_Infantry(id)&&Target_Value(id)<INFANTRY_MAX)||(Is_Target_Aircraft(id)&&Target_Value(id)<AIRCRAFT_MAX);}
bool CombatID(TARGET id){return MobileID(id)||(Is_Target_Building(id)&&Target_Value(id)<BUILDING_MAX);}
bool GroundCell(CELL c)
{
    return c>=0&&c<MAP_CELL_TOTAL&&Map.In_Radar(c)&&Ground[Map[c].Land_Type()].Cost[SPEED_TRACK]
        &&!Map[c].Flag.Occupy.Building&&Map[c].Is_Clear_To_Move(true,true);
}
FootClass* Foot(TARGET id)
{
    auto t=As_Techno(id);if(!t||!t->IsActive)return nullptr;
    auto kind=t->What_Am_I();return kind==RTTI_UNIT||kind==RTTI_INFANTRY||kind==RTTI_AIRCRAFT?static_cast<FootClass*>(t):nullptr;
}
bool Eligible(FootClass const* f)
{
    if(!f||!f->IsActive||f->IsInLimbo||f->Strength<=0||!Campaign_Enemy_House(f->House)
        ||f->Team||f->Trigger||f->IsALoaner||f->In_Radio_Contact()||!Map.In_Radar(Coord_Cell(f->Coord)))return false;
    auto type=f->Techno_Type_Class();if(type->Primary==WEAPON_NONE||type->IsTransporter)return false;
    if(f->What_Am_I()==RTTI_UNIT){auto u=static_cast<UnitClass const*>(f);if(u->Class->IsToHarvest||*u==UNIT_MCV||*u==UNIT_HOVER||*u==UNIT_GUNBOAT)return false;}
    return true;
}
TechnoClass* Target(HouseClass const* h)
{
    for(int i=0;i<Buildings.Count();++i){auto b=Buildings.Ptr(i);if(b->IsActive&&!b->IsInLimbo&&b->Strength>0&&b->House==PlayerPtr)return b;}
    for(int i=0;i<Units.Count();++i){auto u=Units.Ptr(i);if(u->IsActive&&!u->IsInLimbo&&u->Strength>0&&u->House==PlayerPtr&&Map.In_Radar(Coord_Cell(u->Coord)))return u;}
    for(int i=0;i<Infantry.Count();++i){auto u=Infantry.Ptr(i);if(u->IsActive&&!u->IsInLimbo&&u->Strength>0&&u->House==PlayerPtr&&Map.In_Radar(Coord_Cell(u->Coord)))return u;}
    return nullptr;
}
int Distance2(CELL a,CELL b){int dx=Cell_X(a)-Cell_X(b),dy=Cell_Y(a)-Cell_Y(b);return dx*dx+dy*dy;}
std::vector<unsigned char> Reachable(CELL start)
{
    std::vector<unsigned char> seen(MAP_CELL_TOTAL,0);std::queue<CELL> q;seen[start]=1;q.push(start);
    while(!q.empty()){CELL c=q.front();q.pop();int x=Cell_X(c),y=Cell_Y(c);int dx[4]={0,1,0,-1},dy[4]={-1,0,1,0};for(int i=0;i<4;++i){int nx=x+dx[i],ny=y+dy[i];if(nx<0||nx>=MAP_CELL_W||ny<0||ny>=MAP_CELL_H)continue;CELL n=XY_Cell(nx,ny);if(!seen[n]&&GroundCell(n)){seen[n]=1;q.push(n);}}}
    return seen;
}
CELL Rally(FootClass* f,TechnoClass* target,std::vector<unsigned char> const& seen)
{
    CELL start=Coord_Cell(f->Coord),goal=Coord_Cell(target->Coord);
    CELL best=start;int score=std::numeric_limits<int>::min();
    for(int y=MAX(Map.MapCellY,Cell_Y(start)-12);y<=MIN(Map.MapCellY+Map.MapCellHeight-1,Cell_Y(start)+12);++y)
        for(int x=MAX(Map.MapCellX,Cell_X(start)-12);x<=MIN(Map.MapCellX+Map.MapCellWidth-1,Cell_X(start)+12);++x){
            CELL c=XY_Cell(x,y);if(!seen[c]||!GroundCell(c))continue;int open=0;
            for(int dy=-3;dy<=3;++dy)for(int dx=-3;dx<=3;++dx){int nx=x+dx,ny=y+dy;if(nx>=0&&nx<MAP_CELL_W&&ny>=0&&ny<MAP_CELL_H&&GroundCell(XY_Cell(nx,ny)))++open;}
            int s=open*100-Distance2(c,start)*3-Distance2(c,goal)/12;
            if(s>score){score=s;best=c;}
        }
    return best;
}
CELL Slot(FootClass* f,CELL center,int index,std::set<CELL>& reserved,bool direct=false,std::vector<unsigned char> const* reachable=nullptr)
{
    int px=Cell_X(center)+(direct?0:(index%4)*2-3),py=Cell_Y(center)+(direct?0:(index/4)*2-2);
    for(int radius=0;radius<=6;++radius)for(int dy=-radius;dy<=radius;++dy)for(int dx=-radius;dx<=radius;++dx){
        if(MAX(ABS(dx),ABS(dy))!=radius)continue;int x=px+dx,y=py+dy;if(x<0||x>=MAP_CELL_W||y<0||y>=MAP_CELL_H)continue;CELL c=XY_Cell(x,y);
        if(!GroundCell(c)||reserved.count(c)||(reachable&&!(*reachable)[c])||f->Can_Enter_Cell(c)!=MOVE_OK)continue;
        reserved.insert(c);return c;
    }
    return 0;
}
void Move(FootClass* f,CELL cell)
{
    if(!cell||f->SuspendedMission!=MISSION_NONE)return; // Traffic exclusively owns temporary MOVE overrides.
    if(Distance2(Coord_Cell(f->Coord),cell)<=1){if(f->Mission==MISSION_MOVE){f->Assign_Mission(MISSION_GUARD);f->Commence();f->Assign_Destination(TARGET_NONE);}return;}
    if(f->Mission!=MISSION_MOVE||f->NavCom!=::As_Target(cell)){f->Assign_Mission(MISSION_MOVE);f->Commence();f->Assign_Target(TARGET_NONE);f->Assign_Destination(::As_Target(cell));}
}
void Attack(Squad& s,TechnoClass* target)
{
    s.phase=2;
    for(auto m:s.members){auto f=Foot(m.id);if(!Eligible(f)||f->SuspendedMission!=MISSION_NONE)continue;
        f->Assign_Mission(MISSION_HUNT);f->Commence();f->Assign_Target(target->As_Target());f->Assign_Destination(target->As_Target());}
}
void Advance(Squad& s,TechnoClass* target)
{
    s.phase=1;s.deadline=Frame+25*TICKS_PER_SECOND;
    int tx=Cell_X(Coord_Cell(target->Coord)),ty=Cell_Y(Coord_Cell(target->Coord));
    int dx=tx-Cell_X(s.rally),dy=ty-Cell_Y(s.rally),length=MAX(1,MAX(ABS(dx),ABS(dy)));
    int x=Bound(tx-dx*8/length,Map.MapCellX,Map.MapCellX+Map.MapCellWidth-1);
    int y=Bound(ty-dy*8/length,Map.MapCellY,Map.MapCellY+Map.MapCellHeight-1);
    std::set<CELL> reserved;
    // Armor leads the front rows; infantry forms the rear rows.
    std::stable_sort(s.members.begin(),s.members.end(),[](Member a,Member b){auto fa=Foot(a.id),fb=Foot(b.id);bool aa=fa&&fa->What_Am_I()==RTTI_UNIT,ab=fb&&fb->What_Am_I()==RTTI_UNIT;return aa&&!ab;});
    for(unsigned i=0;i<s.members.size();++i){auto f=Foot(s.members[i].id);if(!Eligible(f))continue;auto reachable=Reachable(Coord_Cell(f->Coord));int side=(i%4)*2-3,back=(i/4)*2;int px=Bound(x+(-dy*side-dx*back)/length,Map.MapCellX,Map.MapCellX+Map.MapCellWidth-1),py=Bound(y+(dx*side-dy*back)/length,Map.MapCellY,Map.MapCellY+Map.MapCellHeight-1);auto slot=Slot(f,XY_Cell(px,py),0,reserved,true,reachable.empty()?nullptr:&reachable);s.members[i].slot=slot;if(slot)Move(f,slot);}
}
}
void Campaign_Formation_Reset(){squads.clear();membership.clear();nextID=1;}
void Campaign_Formation_Forget(TARGET id)
{
    auto it=membership.find(id);if(it==membership.end())return;auto group=squads.find(it->second);
    if(group!=squads.end()){auto& members=group->second.members;members.erase(std::remove_if(members.begin(),members.end(),[&](Member m){return m.id==id;}),members.end());if(members.empty())squads.erase(group);}
    membership.erase(it);
}
bool Campaign_Formation_Holding(FootClass const* f)
{
    if(!f)return false;auto member=membership.find(f->As_Target());if(member==membership.end())return false;
    auto group=squads.find(member->second);return group!=squads.end()&&group->second.phase<2;
}
void Campaign_Formation_Recruit(FootClass* f,bool producedCompanion)
{
    if(!Eligible(f)||membership.count(f->As_Target())||f->SuspendedMission!=MISSION_NONE)return;
    switch(f->Mission){case MISSION_GUARD:case MISSION_GUARD_AREA:case MISSION_SLEEP:case MISSION_HUNT:break;case MISSION_MOVE:if(producedCompanion)break;return;default:return;}
    auto target=Target(f->House);if(!target)return;
    auto reachable=Reachable(Coord_Cell(f->Coord));
    unsigned id=0;
    for(auto const& item:squads)if(item.second.house==f->House->Class->House&&item.second.phase==0&&item.second.members.size()<12&&reachable[item.second.rally]&&Distance2(item.second.rally,Coord_Cell(f->Coord))<18*18){id=item.first;break;}
    if(!id){id=nextID++;if(!nextID)nextID=1;squads.emplace(id,Squad{f->House->Class->House,target->As_Target(),Rally(f,target,reachable),Frame+20*TICKS_PER_SECOND,0,{}});}
    auto& s=squads.at(id);std::set<CELL> reserved;for(auto member:s.members)if(member.slot)reserved.insert(member.slot);
    CELL slot=Slot(f,s.rally,s.members.size(),reserved,false,&reachable);s.members.push_back(Member{f->As_Target(),slot});membership[f->As_Target()]=id;Move(f,slot);
    if(f->Distance(target)<=6*ICON_LEPTON_W)Attack(s,target);
}
void Campaign_Formation_AI()
{
    if(GameToPlay!=GAME_NORMAL||!GameActive||ScenarioInit||!PlayerPtr||!PlayerPtr->IsHuman||PlayerWins||PlayerLoses)return;
    // Amortize migration of large existing armies; new factory/wave products recruit immediately.
    int budget=24;
    auto recruit=[&](FootClass* f){if(budget<=0||membership.count(f->As_Target()))return;auto before=membership.size();Campaign_Formation_Recruit(f);if(membership.size()!=before)--budget;};
    for(int i=0;i<Units.Count();++i)recruit(Units.Ptr(i));
    for(int i=0;i<Infantry.Count();++i)recruit(Infantry.Ptr(i));
    for(int i=0;i<Aircraft.Count();++i)recruit(Aircraft.Ptr(i));
    for(auto& item:squads){auto& s=item.second;auto h=HouseClass::As_Pointer(s.house);if(!Campaign_Enemy_House(h))continue;
        auto target=As_Techno(s.target);if(!target||!target->IsActive||target->IsInLimbo||target->Strength<=0||target->House!=PlayerPtr){target=Target(h);if(!target)continue;s.target=target->As_Target();}
        if(s.phase==2){
            // A traffic sidestep can outlive the launch command. Rejoin only those stragglers
            // after their native override ends; do not interrupt combat, repair or transports.
            for(auto member:s.members){auto f=Foot(member.id);if(!Eligible(f)||f->House!=h||f->SuspendedMission!=MISSION_NONE)continue;
                if(f->Mission==MISSION_MOVE||f->Mission==MISSION_GUARD||f->Mission==MISSION_GUARD_AREA||f->Mission==MISSION_SLEEP){f->Assign_Mission(MISSION_HUNT);f->Commence();f->Assign_Target(target->As_Target());f->Assign_Destination(target->As_Target());}}
            continue;
        }
        int ready=0,live=0;bool close=false;
        for(auto member:s.members){auto f=Foot(member.id);if(!Eligible(f)||f->House!=h)continue;++live;if(f->Distance(target)<=6*ICON_LEPTON_W)close=true;if(member.slot&&Distance2(Coord_Cell(f->Coord),member.slot)<=2)++ready;Move(f,member.slot);}
        if(close){Attack(s,target);continue;}
        if(s.phase==0&&((live>=8&&ready>=4)||Frame>=s.deadline))Advance(s,target);
        else if(s.phase==1&&((live&&ready*2>=live)||Frame>=s.deadline))Attack(s,target);
    }
}
bool Campaign_Formation_Save(FileClass& file)
{
    std::uint32_t header[2]={nextID,std::uint32_t(squads.size())};if(file.Write(magic,8)!=8||file.Write(header,sizeof(header))!=sizeof(header))return false;
    for(auto const& entry:squads){auto const& s=entry.second;std::uint32_t data[8]={entry.first,std::uint32_t(s.house),std::uint32_t(s.target),std::uint32_t(s.rally),std::uint32_t(Bound(s.deadline-Frame,0,30*TICKS_PER_SECOND)),s.phase,std::uint32_t(s.members.size()),0};if(file.Write(data,sizeof(data))!=sizeof(data))return false;for(auto m:s.members){std::uint32_t member[2]={std::uint32_t(m.id),std::uint32_t(m.slot)};if(file.Write(member,sizeof(member))!=sizeof(member))return false;}}
    return true;
}
bool Campaign_Formation_Load(FileClass& file)
{
    Campaign_Formation_Reset();char magicRead[8];int n=file.Read(magicRead,8);if(!n)return true;if(n!=8||std::memcmp(magicRead,magic,8))return false;
    std::uint32_t header[2];if(file.Read(header,sizeof(header))!=sizeof(header)||!header[0]||header[1]>UNIT_MAX+INFANTRY_MAX+AIRCRAFT_MAX)return false;nextID=header[0];
    for(unsigned i=0;i<header[1];++i){std::uint32_t d[8];if(file.Read(d,sizeof(d))!=sizeof(d)||!d[0]||d[0]>=nextID||squads.count(d[0])||d[1]>=HOUSE_COUNT||!CombatID(TARGET(d[2]))||!d[3]||d[3]>=MAP_CELL_TOTAL||d[4]>30*TICKS_PER_SECOND||d[5]>2||d[6]>12||d[7])return false;Squad s{HousesType(d[1]),TARGET(d[2]),CELL(d[3]),Frame+int(d[4]),d[5],{}};for(unsigned j=0;j<d[6];++j){std::uint32_t m[2];if(file.Read(m,sizeof(m))!=sizeof(m)||m[1]>=MAP_CELL_TOTAL||membership.count(m[0])||!MobileID(TARGET(m[0])))return false;membership[m[0]]=d[0];s.members.push_back(Member{TARGET(m[0]),CELL(m[1])});}squads.emplace(d[0],s);}
    return true;
}
bool Campaign_Construction_Preserves_Paths(BuildingTypeClass const& type,CELL cell)
{
    std::set<CELL> footprint;int xmin=MAP_CELL_W,xmax=0,ymin=MAP_CELL_H,ymax=0;
    for(auto p=type.Occupy_List(true);*p!=REFRESH_EOL;++p){CELL c=cell+*p;if(!Map.In_Radar(c))return false;footprint.insert(c);xmin=MIN(xmin,Cell_X(c));xmax=MAX(xmax,Cell_X(c));ymin=MIN(ymin,Cell_Y(c));ymax=MAX(ymax,Cell_Y(c));}
    xmin=MAX(Map.MapCellX,xmin-6);xmax=MIN(Map.MapCellX+Map.MapCellWidth-1,xmax+6);ymin=MAX(Map.MapCellY,ymin-6);ymax=MIN(Map.MapCellY+Map.MapCellHeight-1,ymax+6);
    std::set<CELL> ports;int dx[4]={0,1,0,-1},dy[4]={-1,0,1,0};
    for(CELL c:footprint)for(int i=0;i<4;++i){int x=Cell_X(c)+dx[i],y=Cell_Y(c)+dy[i];if(x<xmin||x>xmax||y<ymin||y>ymax)continue;CELL n=XY_Cell(x,y);if(!footprint.count(n)&&GroundCell(n))ports.insert(n);}
    if(ports.size()<2)return true;
    std::set<CELL> visited;std::queue<CELL> q;CELL start=*ports.begin();q.push(start);visited.insert(start);
    while(!q.empty()){CELL c=q.front();q.pop();for(int i=0;i<4;++i){int x=Cell_X(c)+dx[i],y=Cell_Y(c)+dy[i];if(x<xmin||x>xmax||y<ymin||y>ymax)continue;CELL n=XY_Cell(x,y);if(!visited.count(n)&&!footprint.count(n)&&GroundCell(n)){visited.insert(n);q.push(n);}}}
    for(CELL c:ports)if(!visited.count(c))return false;
    return true;
}
