// Native offline combat probe: temporarily replaces missionstarts.cpp only.
#include "function.h"
#include "campaigndefense.h"
#include <cstdio>
#include <string>
static int failures;
static void Check(const char* name,bool ok){std::printf("%s %s\n",ok?"PASS":"FAIL",name);std::fflush(stdout);if(!ok)++failures;}
bool Generate_Mission_Starts(const char* directory){
 std::string dir(directory);
 for(int kind=0;kind<4;++kind){
  if(!Load_Game((dir+"/fixture.sav").c_str()))return false;
  for(int i=0;i<Triggers.Count();++i)Triggers.Ptr(i)->Action=TriggerClass::ACTION_NONE;
  Options.Set();GameToPlay=GAME_NORMAL;GameActive=true;PlayerWins=PlayerLoses=false;
  HouseClass* good=HouseClass::As_Pointer(HOUSE_GOOD);HouseClass* bad=HouseClass::As_Pointer(HOUSE_BAD);
  good->IsHuman=true;bad->IsHuman=false;PlayerPtr=good;good->Power=bad->Power=1000000;
  for(int i=Units.Count()-1;i>=0;--i)Units.Ptr(i)->Limbo();
  for(int i=Infantry.Count()-1;i>=0;--i)Infantry.Ptr(i)->Limbo();
  for(int i=Buildings.Count()-1;i>=0;--i)Buildings.Ptr(i)->Limbo();
  for(int i=Aircraft.Count()-1;i>=0;--i)Aircraft.Ptr(i)->Limbo();
  BuildingClass* sam=new BuildingClass(STRUCT_SAM,HOUSE_GOOD);
  BuildingClass* enemy=new BuildingClass(STRUCT_SAM,HOUSE_BAD);
  TechnoClass* target=kind==0?static_cast<TechnoClass*>(new UnitClass(UNIT_MTANK,HOUSE_BAD)):
                       kind==1?static_cast<TechnoClass*>(new InfantryClass(INFANTRY_E1,HOUSE_BAD)):
                       kind==2?static_cast<TechnoClass*>(new BuildingClass(STRUCT_POWER,HOUSE_BAD)):
                               static_cast<TechnoClass*>(new AircraftClass(AIRCRAFT_ORCA,HOUSE_BAD));
  int x=0,y=0;bool placed=false;
  for(int cy=Map.MapCellY+3;cy<Map.MapCellY+Map.MapCellHeight-4&&!placed;++cy)
   for(int cx=Map.MapCellX+2;cx<Map.MapCellX+Map.MapCellWidth-24;++cx)
    if(sam->Can_Enter_Cell(XY_Cell(cx,cy))==MOVE_OK&&Map[XY_Cell(cx+20,cy)].Is_Generally_Clear()
       && (kind!=2||static_cast<BuildingClass*>(target)->Can_Enter_Cell(XY_Cell(cx+20,cy))==MOVE_OK)){
     x=cx;y=cy;placed=true;break;
    }
  Check("clear twenty-cell combat lane",placed);if(!placed)return false;
  Check("place player SAM",sam->Unlimbo(Cell_Coord(XY_Cell(x,y)),DIR_E));
  Check("place target at twenty cells",target->Unlimbo(Cell_Coord(XY_Cell(x+20,y)),DIR_N));
  target->IsDiscoveredByPlayer=true;
  if(kind==3)static_cast<AircraftClass*>(target)->Altitude=AircraftClass::FLIGHT_LEVEL;
  int base=Weapons[WEAPON_NIKE].Range;
  Check("player SAM range equals six times original",sam->Weapon_Range(0)==base*6);
  Check("enemy SAM retains previous range",enemy->Weapon_Range(0)==base*2);
  Check("SAM damage and reload remain unchanged",Weapons[WEAPON_NIKE].Attack==50&&Weapons[WEAPON_NIKE].ROF==50);
  Check("automatic scan acquires far target",sam->Greatest_Threat(THREAT_NORMAL)==target->As_Target());
  Check("manual attack action is available",sam->What_Action(target)==ACTION_ATTACK);
  sam->Assign_Target(TARGET_NONE);sam->Mission_Guard();
  Check("idle guard assigns target",sam->TarCom==target->As_Target());
  int before=target->Strength;int fired=0;
  sam->Assign_Mission(MISSION_ATTACK);sam->Commence();
  for(int tick=0;tick<2500&&target->IsActive&&!target->IsInLimbo&&target->Strength==before;++tick){
   ++Frame;sam->AI();
   if(Bullets.Count())++fired;
   for(int i=Bullets.Count()-1;i>=0;--i){BulletClass* b=Bullets.Ptr(i);if(b->IsActive&&!b->IsInLimbo)b->AI();}
  }
  if(!fired)std::printf("DEBUG kind=%d status=%d stage=%d mission=%d target=%u canfire=%d range=%d locked=%d\n",kind,sam->Status,sam->Fetch_Stage(),sam->Mission,unsigned(sam->TarCom),sam->Can_Fire(target->As_Target(),0),sam->Weapon_Range(0),sam->IsLocked);
  Check("native SAM animation and attack actually launch missiles",fired>0);
  if(kind!=1)Check("missile reaches and damages far target",target->Strength<before||!target->IsActive);
  sam->House=bad;target->House=good;PlayerPtr=good;
  Check("enemy SAM does not gain ground ability",!Campaign_SAM_Ground(sam));
  if(kind!=3)Check("enemy SAM auto scan ignores ground target",sam->Greatest_Threat(THREAT_NORMAL)==TARGET_NONE);
  if(kind!=3)Check("enemy SAM rejects ground target state",!Campaign_SAM_Target_Allowed(sam,target->As_Target()));
  sam->House=good;target->House=bad;
  COORDINATE old=target->Coord;target->Coord=Coord_Move(sam->Fire_Coord(0),DIR_E,base*6+4*256);
  Check("beyond sixfold range is excluded",!sam->In_Range(target->As_Target(),0));target->Coord=old;
  GameToPlay=GAME_SKIRMISH;Check("skirmish has no new perk",!Campaign_SAM_Ground(sam)&&sam->Weapon_Range(0)==base);GameToPlay=GAME_NORMAL;
  good->IsHuman=false;Check("AI player slot gets no ground perk",!Campaign_SAM_Ground(sam));good->IsHuman=true;
  PlayerPtr=bad;bad->IsHuman=true;Check("Nod owned SAM gets same perk",Campaign_SAM_Ground(enemy)&&enemy->Weapon_Range(0)==base*6);bad->IsHuman=false;PlayerPtr=good;
  if(kind==0){
   sam->Assign_Target(TARGET_NONE);sam->Arm=0;sam->Assign_Mission(MISSION_GUARD);sam->Commence();
   target->Strength=target->Class_Of().MaxStrength;
   for(int i=0;i<MAP_CELL_TOTAL;++i)if(Map.In_Radar(i))Map.Map_Cell(i,good,true);
   ++ScenarioInit;Map.Set_Tactical_Position(Cell_Coord(XY_Cell(x-1,y-3)));--ScenarioInit;
   target->Assign_Target(TARGET_NONE);target->Assign_Mission(MISSION_SLEEP);target->Commence();
   good->Credits=50000;good->IsToWin=good->IsToLose=good->IsToDie=false;
   Check("save isolated ground-attack preview",Save_Game((dir+"/savegame.095").c_str(),"SAM GROUND ATTACK PREVIEW"));
   Check("save reload keeps current-owned SAM perk",Load_Game((dir+"/savegame.095").c_str()));
   bool found=false;for(int i=0;i<Buildings.Count();++i){auto b=Buildings.Ptr(i);if(!b->IsInLimbo&&b->House==PlayerPtr&&b->Class->Type==STRUCT_SAM){found=true;Check("loaded SAM still acquires ground target",b->Weapon_Range(0)==base*6&&Target_Legal(b->Greatest_Threat(THREAT_NORMAL)));}}
   Check("loaded SAM found",found);
  }
 }
 std::printf("TOTAL FAILURES %d\n",failures);return failures==0;
}
