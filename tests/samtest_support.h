// Shared native SAM integration fixtures, never used by the shipped game.
// Native private SAM damage/blast probe. Never ship this entry point.
#include "function.h"
#include "campaigndefense.h"
#include <cstdio>
#include <string>
#include <vector>
#include <fstream>
#include "campaignenemies.h"
static int failures;
static void Check(const char* name,bool ok){std::printf("%s %s\n",ok?"PASS":"FAIL",name);std::fflush(stdout);if(!ok)++failures;}
static void Reset(const std::string& dir){Check("load blast fixture",Load_Game((dir+"/fixture.sav").c_str()));Options.Set();GameToPlay=GAME_NORMAL;GameActive=true;PlayerWins=PlayerLoses=false;Frame=100;for(int i=0;i<Triggers.Count();++i)Triggers.Ptr(i)->Action=TriggerClass::ACTION_NONE;for(int i=Units.Count()-1;i>=0;--i)Units.Ptr(i)->Limbo();for(int i=Infantry.Count()-1;i>=0;--i)Infantry.Ptr(i)->Limbo();for(int i=Buildings.Count()-1;i>=0;--i)Buildings.Ptr(i)->Limbo();for(int i=Aircraft.Count()-1;i>=0;--i)Aircraft.Ptr(i)->Limbo();auto good=HouseClass::As_Pointer(HOUSE_GOOD),bad=HouseClass::As_Pointer(HOUSE_BAD);PlayerPtr=good;good->IsHuman=true;bad->IsHuman=false;good->FirepowerBias=bad->FirepowerBias=1;good->Power=bad->Power=1000000;}
static bool Put(TechnoClass* obj,CELL prefer){for(int radius=0;radius<10;++radius)for(int dy=-radius;dy<=radius;++dy)for(int dx=-radius;dx<=radius;++dx){if(MAX(ABS(dx),ABS(dy))!=radius)continue;int x=Cell_X(prefer)+dx,y=Cell_Y(prefer)+dy;if(x<0||x>=MAP_CELL_W||y<0||y>=MAP_CELL_H)continue;CELL cell=XY_Cell(x,y);if(Map.In_Radar(cell)&&obj->Can_Enter_Cell(cell)==MOVE_OK&&obj->Unlimbo(Cell_Coord(cell),DIR_N))return true;}return false;}
static CELL Center(){return XY_Cell(Map.MapCellX+Map.MapCellWidth/2,Map.MapCellY+Map.MapCellHeight/2);}
static BuildingClass* Source(bool player){auto s=new BuildingClass(STRUCT_SAM,player?HOUSE_GOOD:HOUSE_BAD);Check("place SAM away from blast",Put(s,XY_Cell(Map.MapCellX+2,Map.MapCellY+2)));return s;}
static void TickBullets(){std::vector<TARGET> ids;for(int i=0;i<Bullets.Count();++i)ids.push_back(Bullets.Ptr(i)->As_Target());++Frame;for(auto id:ids){auto b=As_Bullet(id);if(b)b->AI();}}

#include <set>
#include <cstdint>
#include <cstring>
static std::vector<TARGET> actors;
static void Actors(bool air=false,int count=13){
 actors.clear();auto c=Center();
 for(int i=0;i<count;++i){TechnoClass* a=air?static_cast<TechnoClass*>(new AircraftClass(AIRCRAFT_ORCA,HOUSE_BAD)):static_cast<TechnoClass*>(new UnitClass(UNIT_MTANK,HOUSE_BAD));
  if(air)static_cast<AircraftClass*>(a)->Altitude=AircraftClass::FLIGHT_LEVEL;
  Check("place chain target",Put(a,XY_Cell(Cell_X(c)+(i%4)*2,Cell_Y(c)+(i/4)*2)));a->Strength=30000;a->IsDiscoveredByPlayer=true;Campaign_Enemies_Mark_Extra(a);actors.push_back(a->As_Target());}
}
static int Loss(){int sum=0;for(auto id:actors){auto a=As_Techno(id);if(a)sum+=30000-a->Strength;}return sum;}
static TechnoClass* Aim(BulletClass* b){TechnoClass* best=nullptr;int distance=0x7fffffff;for(auto id:actors){auto a=As_Techno(id);if(!a||a->IsInLimbo)continue;int d=Distance(b->Fuse_Target(),a->Target_Coord());if(d<distance){best=a;distance=d;}}return best;}
static void Checkpoint(const std::string& dir){int count=Bullets.Count();Check("save ricochet position",Save_Game((dir+"/branch.sav").c_str(),"FIVE SAM RICOCHETS"));Check("reload ricochet position",Load_Game((dir+"/branch.sav").c_str()));Check("checkpoint preserves each single travelling missile",Bullets.Count()==count);}
static void ImpactGeneration(bool hit){std::vector<TARGET> ids;for(int i=0;i<Bullets.Count();++i)ids.push_back(Bullets.Ptr(i)->As_Target());for(auto id:ids){auto b=As_Bullet(id);auto a=Aim(b);Check("generation has target",a!=nullptr);if(!a)continue;b->Coord=hit?a->Target_Coord():XY_Coord(Coord_X(a->Target_Coord()),Coord_Y(a->Target_Coord())+2*ICON_LEPTON_W);Check("player impact is handled without splash",Campaign_SAM_Chain_Impact(b,a));delete b;}}
