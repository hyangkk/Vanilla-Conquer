#include "function.h"
#include <cstdio>
#include <string>
static int failures=0;
static void Check(const char* label,bool ok) {std::printf("%s %s\n",ok?"PASS":"FAIL",label);std::fflush(stdout);if(!ok)++failures;}
bool Generate_Mission_Starts(const char* directory) {
 std::string save=std::string(directory)+"/fixture.sav";
 for(StructType type : {STRUCT_ATOWER,STRUCT_OBELISK}) {
  if(!Load_Game(save.c_str())) return false;
  GameToPlay=GAME_NORMAL;GameActive=true;
  HouseClass* good=HouseClass::As_Pointer(HOUSE_GOOD);HouseClass* bad=HouseClass::As_Pointer(HOUSE_BAD);
  good->IsHuman=true;bad->IsHuman=false;PlayerPtr=good;good->Power=bad->Power=1000000;
  for(int i=Units.Count()-1;i>=0;--i) Units.Ptr(i)->Limbo();
  for(int i=Infantry.Count()-1;i>=0;--i) Infantry.Ptr(i)->Limbo();
  for(int i=Buildings.Count()-1;i>=0;--i) Buildings.Ptr(i)->Limbo();
  BuildingClass* defense=new BuildingClass(type,HOUSE_GOOD);
  BuildingClass* target=new BuildingClass(STRUCT_POWER,HOUSE_BAD);
  int x=0,y=0;bool placed=false;
  for(int cy=Map.MapCellY+2;cy<Map.MapCellY+Map.MapCellHeight-3&&!placed;++cy) {
   for(int cx=Map.MapCellX+2;cx<Map.MapCellX+Map.MapCellWidth-13;++cx) {
    if(defense->Can_Enter_Cell(XY_Cell(cx,cy))==MOVE_OK && target->Can_Enter_Cell(XY_Cell(cx+10,cy))==MOVE_OK) {x=cx;y=cy;placed=true;break;}
   }
  }
  Check("find clear placement pair",placed);if(!placed)return false;
  Check("place actual player defense",defense->Unlimbo(Cell_Coord(XY_Cell(x,y)),DIR_N));
  Check("place actual unarmed enemy building",target->Unlimbo(Cell_Coord(XY_Cell(x+10,y)),DIR_N));
  target->IsDiscoveredByPlayer=true; defense->IsCharged=true;defense->Arm=0;
  int value=0;
  Check("unarmed enemy building is eligible",defense->Evaluate_Object(THREAT_BUILDINGS,1<<RTTI_BUILDING,0,target,value));
  Check("guard scan acquires enemy building at 10 cells",defense->Greatest_Threat(THREAT_NORMAL)==target->As_Target());
  defense->Assign_Target(TARGET_NONE);defense->Mission_Guard();
  Check("idle guard automatically assigns enemy building",defense->TarCom==target->As_Target());
  Check("building target can be fired upon",defense->Can_Fire(target->As_Target(),0)==FIRE_OK);
  int before=target->Strength;
  int bulletCount=Bullets.Count();
  defense->Mission_Attack();
  Check("native attack mission fires at automatic building target",Bullets.Count()>bulletCount);
  for(int tick=0;tick<400;++tick) {
   ++Frame;
   for(int i=Bullets.Count()-1;i>=0;--i) {BulletClass* b=Bullets.Ptr(i);if(b->IsActive&&!b->IsInLimbo)b->AI();}
  }
  Check("projectile damages enemy building",target->Strength<before);
  target->IsDiscoveredByPlayer=false;value=0;
  Check("unseen building excluded",!defense->Evaluate_Object(THREAT_BUILDINGS,1<<RTTI_BUILDING,0,target,value));
  target->IsDiscoveredByPlayer=true;target->House=good;
  Check("friendly building excluded",!defense->Evaluate_Object(THREAT_BUILDINGS,1<<RTTI_BUILDING,0,target,value));
  target->House=bad;
  COORDINATE old=target->Coord;target->Coord=XY_Coord(Coord_X(defense->Coord)+25*256,Coord_Y(defense->Coord));
  Check("out-of-range building excluded",!defense->Evaluate_Object(THREAT_BUILDINGS,1<<RTTI_BUILDING,0,target,value));target->Coord=old;
  BuildingClass* ordinary=new BuildingClass(STRUCT_GTOWER,HOUSE_GOOD);ordinary->Coord=defense->Coord;ordinary->IsLocked=true;
  target->Coord=XY_Coord(Coord_X(defense->Coord)+2*256,Coord_Y(defense->Coord));
  Check("ordinary guard tower keeps unarmed building exclusion",!ordinary->Evaluate_Object(THREAT_BUILDINGS,1<<RTTI_BUILDING,0,target,value));
  GameToPlay=GAME_SKIRMISH;
  Check("skirmish keeps unarmed building exclusion",!defense->Evaluate_Object(THREAT_BUILDINGS,1<<RTTI_BUILDING,0,target,value));GameToPlay=GAME_NORMAL;
  good->IsHuman=true;bad->IsHuman=true;PlayerPtr=bad;
  Check("non-player human defense keeps exclusion",!defense->Evaluate_Object(THREAT_BUILDINGS,1<<RTTI_BUILDING,0,target,value));
  defense->House=bad;target->House=good;
  Check("Nod player defense accepts enemy building",defense->Evaluate_Object(THREAT_BUILDINGS,1<<RTTI_BUILDING,0,target,value));
  PlayerPtr=good;good->IsHuman=true;bad->IsHuman=false;
  Check("enemy AI still accepts hostile unarmed buildings",defense->Evaluate_Object(THREAT_BUILDINGS,1<<RTTI_BUILDING,0,target,value));

 }
 std::printf("TOTAL FAILURES %d\n",failures);return failures==0;
}
