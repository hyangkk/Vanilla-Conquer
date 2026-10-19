// Native offline combat probe: temporarily replaces missionstarts.cpp only.
#include "function.h"
#include "campaigndefense.h"
#include <cstdio>
#include <string>
#include <vector>
static int failures;
static void Check(const char* name,bool ok){std::printf("%s %s\n",ok?"PASS":"FAIL",name);std::fflush(stdout);if(!ok)++failures;}
bool Ground_Check(const char* directory){
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
   for(int cx=Map.MapCellX+2;cx<Map.MapCellX+Map.MapCellWidth-44;++cx)
    if(sam->Can_Enter_Cell(XY_Cell(cx,cy))==MOVE_OK&&Map[XY_Cell(cx+40,cy)].Is_Generally_Clear()
       && target->Can_Enter_Cell(XY_Cell(cx+40,cy))==MOVE_OK){
     x=cx;y=cy;placed=true;break;
    }
  Check("clear forty-cell combat lane",placed);if(!placed)return false;
  Check("place player SAM",sam->Unlimbo(Cell_Coord(XY_Cell(x,y)),DIR_E));
  Check("place target at forty cells",target->Unlimbo(Cell_Coord(XY_Cell(x+40,y)),DIR_N));
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
   auto clearSight=[](){for(int i=0;i<MAP_CELL_TOTAL;++i){Map[i].IsMapped=false;Map[i].IsVisible=false;Map[i].IsMappedByPlayerMask=0;Map[i].IsVisibleByPlayerMask=0;}};
   CELL center=Coord_Cell(sam->Center_Coord());
   clearSight();sam->Look();std::vector<bool> expected;
   for(int i=0;i<MAP_CELL_TOTAL;++i)expected.push_back(Map[i].Is_Mapped(good));
   Check("sight reveals twenty-nine cells east",Map[XY_Cell(Cell_X(center)+29,Cell_Y(center))].Is_Mapped(good));
   Check("sight excludes thirty-four cells east",!Map[XY_Cell(Cell_X(center)+34,Cell_Y(center))].Is_Mapped(good));
   clearSight();for(int i=0;i<MAP_CELL_TOTAL;++i)if(Distance(CELL(i),center)<=30)Map.Map_Cell(i,good,true);
   bool exact=true;for(int i=0;i<MAP_CELL_TOTAL;++i)if(expected[i]!=Map[i].Is_Mapped(good))exact=false;
   Check("player SAM sight matches full thirty-cell radius",exact);
   clearSight();sam->House=bad;sam->Look();
   Check("enemy SAM does not reveal twenty-nine cells",!Map[XY_Cell(Cell_X(center)+29,Cell_Y(center))].Is_Mapped(bad));sam->House=good;
   clearSight();Check("save existing SAM with unrevealed fog",Save_Game((dir+"/sight-load.sav").c_str(),"SAM SIGHT LOAD CHECK"));
   Check("reload existing SAM",Load_Game((dir+"/sight-load.sav").c_str()));
   bool contains=true;for(int i=0;i<MAP_CELL_TOTAL;++i)if(expected[i]&&!Map[i].Is_Mapped(PlayerPtr))contains=false;
   Check("existing save reveals full thirty-cell radius",contains);
   // Resolve pooled actors after native load, preserving the scenario camera below.
   sam=nullptr;target=nullptr;
   for(int i=0;i<Buildings.Count();++i)if(!Buildings.Ptr(i)->IsInLimbo&&Buildings.Ptr(i)->House==PlayerPtr&&Buildings.Ptr(i)->Class->Type==STRUCT_SAM)sam=Buildings.Ptr(i);
   for(int i=0;i<Units.Count();++i)if(!Units.Ptr(i)->IsInLimbo&&Units.Ptr(i)->House!=PlayerPtr)target=Units.Ptr(i);
   if(!sam||!target)return false;

   sam->Assign_Target(TARGET_NONE);sam->Arm=0;sam->Assign_Mission(MISSION_GUARD);sam->Commence();
   target->Strength=target->Class_Of().MaxStrength;
   // Keep the visible target beyond the previous 22.5-cell range, on this canvas.
   for(int i=0;i<Units.Count();++i)if(Units.Ptr(i)!=target&&!Units.Ptr(i)->IsInLimbo)Units.Ptr(i)->Limbo();
   target->Limbo();bool previewPlaced=false;
   for(int dx=27;dx<=30&&!previewPlaced;++dx)for(int dy=0;dy<8&&!previewPlaced;++dy){
    CELL cell=XY_Cell(x+dx,y+dy);
    if(target->Can_Enter_Cell(cell)==MOVE_OK)previewPlaced=target->Unlimbo(Cell_Coord(cell),DIR_N);
   }
   Check("place visible target beyond previous range",previewPlaced);target->Strength=20000;
   clearSight();sam->Look();
   ++ScenarioInit;Map.Set_Tactical_Position(Cell_Coord(XY_Cell(x-1,y-3)));--ScenarioInit;
   target->Assign_Target(TARGET_NONE);target->Assign_Mission(MISSION_SLEEP);target->Commence();
   good->Credits=50000;good->IsToWin=good->IsToLose=good->IsToDie=false;
   Check("save isolated ground-attack preview",Save_Game((dir+"/savegame.098").c_str(),"SAM RANGE 45 SPEED X2"));
   Check("save reload keeps current-owned SAM perk",Load_Game((dir+"/savegame.098").c_str()));
   bool found=false;for(int i=0;i<Buildings.Count();++i){auto b=Buildings.Ptr(i);if(!b->IsInLimbo&&b->House==PlayerPtr&&b->Class->Type==STRUCT_SAM){found=true;Check("loaded SAM still acquires ground target",b->Weapon_Range(0)==base*6&&Target_Legal(b->Greatest_Threat(THREAT_NORMAL)));}}
   Check("loaded SAM found",found);
  }
 }
 std::printf("TOTAL FAILURES %d\n",failures);return failures==0;
}

static int Cadence(const char* directory,bool player){
 std::string dir(directory);if(!Load_Game((dir+"/fixture.sav").c_str()))return 0;
 for(int i=0;i<Triggers.Count();++i)Triggers.Ptr(i)->Action=TriggerClass::ACTION_NONE;
 Options.Set();GameToPlay=GAME_NORMAL;GameActive=true;PlayerWins=PlayerLoses=false;
 auto good=HouseClass::As_Pointer(HOUSE_GOOD),bad=HouseClass::As_Pointer(HOUSE_BAD);PlayerPtr=good;good->IsHuman=true;bad->IsHuman=false;bad->ROFBias=good->ROFBias;
 for(int i=Units.Count()-1;i>=0;--i)Units.Ptr(i)->Limbo();
 for(int i=Infantry.Count()-1;i>=0;--i)Infantry.Ptr(i)->Limbo();
 for(int i=Buildings.Count()-1;i>=0;--i)Buildings.Ptr(i)->Limbo();
 for(int i=Aircraft.Count()-1;i>=0;--i)Aircraft.Ptr(i)->Limbo();
 auto sam=new BuildingClass(STRUCT_SAM,player?HOUSE_GOOD:HOUSE_BAD);
 auto air=new AircraftClass(AIRCRAFT_ORCA,player?HOUSE_BAD:HOUSE_GOOD);
 bool placed=false;
 for(int y=Map.MapCellY+13;y<Map.MapCellY+Map.MapCellHeight-4&&!placed;++y)
  for(int x=Map.MapCellX+3;x<Map.MapCellX+Map.MapCellWidth-4;++x)
   if(sam->Can_Enter_Cell(XY_Cell(x,y))==MOVE_OK&&Map[XY_Cell(x,y-10)].Is_Generally_Clear()&&air->Can_Enter_Cell(XY_Cell(x,y-10))==MOVE_OK&&sam->Unlimbo(Cell_Coord(XY_Cell(x,y)),DIR_N)){placed=air->Unlimbo(Cell_Coord(XY_Cell(x,y-10)),DIR_N);break;}
 Check("place cadence SAM and stationary airborne target",placed);if(!placed)return 0;
 air->Altitude=AircraftClass::FLIGHT_LEVEL;air->Strength=20000;air->IsDiscoveredByPlayer=true;
 sam->Assign_Target(air->As_Target());sam->Assign_Mission(MISSION_ATTACK);sam->Commence();
 std::vector<int> shots;
 for(int tick=0;tick<10000&&shots.size()<34;++tick){
  ++Frame;int before=Bullets.Count();sam->AI();
  if(Bullets.Count()>before)shots.push_back(tick);
  for(int i=Bullets.Count()-1;i>=0;--i)delete Bullets.Ptr(i);
 }
 Check("sustained native SAM attack fires thirty-four missiles",shots.size()==34);if(shots.size()!=34)return 0;
 int duration=shots[33]-shots[3];
 std::printf("CADENCE %s ticks=%d first=%d last=%d rearm=%d,%d\n",player?"PLAYER":"ENEMY",duration,shots[3],shots[33],sam->Rearm_Delay(false),sam->Rearm_Delay(true));
 int normal=(int)Weapons[WEAPON_NIKE].ROF*sam->House->ROFBias+3;
 Check("SAM rearm delay follows owner only",sam->Rearm_Delay(false)==(player?4:9)&&sam->Rearm_Delay(true)==(player?normal/2:normal));
 return duration;
}
bool Generate_Mission_Starts(const char* directory){
 Ground_Check(directory);
 int oldTicks=Cadence(directory,false),newTicks=Cadence(directory,true);
 double speed=newTicks?double(oldTicks)/newTicks:0;std::printf("SUSTAINED SPEED MULTIPLIER %.6f\n",speed);
 Check("native sustained firing runs twice as fast within tick rounding",speed>1.95&&speed<2.05);
 std::printf("TOTAL FAILURES %d\n",failures);return failures==0;
}
