// Private native production/combat probe; never ship this entry point.
#include "function.h"
#include "campaigntech.h"
#include "campaigndefense.h"
#include <cstdio>
#include <string>
#include <vector>
static int failures;
static void Check(const char* name,bool ok){std::printf("%s %s\n",ok?"PASS":"FAIL",name);std::fflush(stdout);if(!ok)++failures;}
static void Ready(){GameToPlay=GAME_NORMAL;GameActive=true;PlayerWins=PlayerLoses=false;PlayerPtr->IsToWin=PlayerPtr->IsToLose=PlayerPtr->IsToDie=false;for(int i=0;i<Triggers.Count();++i)Triggers.Ptr(i)->Action=TriggerClass::ACTION_NONE;}
static void Clear(){for(int i=Units.Count()-1;i>=0;--i)Units.Ptr(i)->Limbo();for(int i=Infantry.Count()-1;i>=0;--i)Infantry.Ptr(i)->Limbo();for(int i=Buildings.Count()-1;i>=0;--i)Buildings.Ptr(i)->Limbo();for(int i=Aircraft.Count()-1;i>=0;--i)Aircraft.Ptr(i)->Limbo();}
static bool Put(TechnoClass* obj){for(int y=Map.MapCellY+4;y<Map.MapCellY+Map.MapCellHeight-4;++y)for(int x=Map.MapCellX+4;x<Map.MapCellX+Map.MapCellWidth-4;++x)if(obj->Can_Enter_Cell(XY_Cell(x,y))==MOVE_OK&&obj->Unlimbo(Cell_Coord(XY_Cell(x,y)),DIR_N))return true;return false;}
static CELL placedBuildingCell;
static bool PlaceBuilding(HouseClass* house,BuildingClass* obj){for(int y=Map.MapCellY;y<Map.MapCellY+Map.MapCellHeight;++y)for(int x=Map.MapCellX;x<Map.MapCellX+Map.MapCellWidth;++x){CELL c=XY_Cell(x,y);if(obj->Class->Legal_Placement(c)&&house->Place_Object(RTTI_BUILDINGTYPE,c)){placedBuildingCell=c;return true;}}return false;}
static void Production(const std::string& dir,HousesType faction){
 Check("load no-base production fixture",Load_Game((dir+"/fixture.sav").c_str()));Ready();Clear();
 PlayerPtr=HouseClass::As_Pointer(faction);PlayerPtr->IsHuman=true;Frame=100;PlayerPtr->Credits=500000;PlayerPtr->Tiberium=0;
 auto anchor=new InfantryClass(INFANTRY_E1,faction);Check("place lone soldier without MCV or base",Put(anchor));
 PlayerPtr->BuildingFactories=PlayerPtr->UnitFactories=PlayerPtr->InfantryFactories=PlayerPtr->AircraftFactories=0;
 PlayerPtr->IsRecalcNeeded=true;PlayerPtr->AI();Map.Recalc();
 const RTTIType kinds[]={RTTI_BUILDINGTYPE,RTTI_UNITTYPE,RTTI_INFANTRYTYPE,RTTI_AIRCRAFTTYPE};const int counts[]={STRUCT_COUNT,UNIT_COUNT,INFANTRY_COUNT,AIRCRAFT_COUNT};
 int total=0;
 for(int k=0;k<4;++k)for(int id=0;id<counts[k];++id){auto type=Fetch_Techno_Type(kinds[k],id);if(!Campaign_All_Tech_Type(type,PlayerPtr))continue;++total;
  Check("buildable remains in sidebar without physical factory",!Map.Add(kinds[k],id));
  PlayerPtr->Credits=500000;PlayerPtr->Tiberium=0;int cash=PlayerPtr->Available_Money();
  Check("begin production without construction yard or factory",PlayerPtr->Begin_Production(kinds[k],id)==PROD_OK);
  auto factory=PlayerPtr->Fetch_Factory(kinds[k]);if(!factory){Check("production manager exists",false);continue;}
  auto obj=factory->Get_Object();bool wall=k==0&&static_cast<BuildingClass*>(obj)->Class->IsWall;int price=obj->PurchasePrice;bool defense=k==0&&(id==STRUCT_SAM||id==STRUCT_ATOWER||id==STRUCT_OBELISK);
  int ticks=0;while(!factory->Has_Completed()&&ticks<10000){++Frame;factory->AI();++ticks;
   if(defense&&ticks==5){int stage=factory->Completion();Check("save one-second production in progress",Save_Game((dir+"/production.sav").c_str(),"ONE SECOND RESUME"));Check("reload one-second production",Load_Game((dir+"/production.sav").c_str()));Ready();factory=PlayerPtr->Fetch_Factory(kinds[k]);obj=factory->Get_Object();Check("saved production stage preserved",factory->Completion()==stage);}
  }
  std::printf("PRODUCE faction=%d kind=%d id=%d ticks=%d\n",int(faction),int(kinds[k]),id,ticks);
  Check("production finishes without physical factory",factory->Has_Completed());if(defense)Check("defense completes in exactly fifteen game ticks",ticks==TICKS_PER_SECOND);
  Check("unchanged production price",PlayerPtr->Available_Money()==cash-price);
  if(k==0){auto b=static_cast<BuildingClass*>(obj);Check("manual placement accepts missing construction yard",PlayerPtr->Manual_Place(nullptr,b));bool placed=PlaceBuilding(PlayerPtr,b);Check("completed building legally places without construction yard",placed);if(!placed){PlayerPtr->Abandon_Production(kinds[k]);Map.PendingObject=nullptr;Map.PendingObjectPtr=nullptr;return;}}
  else {bool placed=PlayerPtr->Place_Object(kinds[k],CELL(-1));Check("completed mobile unit exits without production building",placed);if(!placed){PlayerPtr->Abandon_Production(kinds[k]);return;}}
  if(wall)Check("wall production becomes native map overlay",Map[placedBuildingCell].Overlay!=OVERLAY_NONE);else {Check("produced object is real and owned by player",obj->IsActive&&!obj->IsInLimbo&&obj->House==PlayerPtr);if(obj->IsActive)delete obj;}
 }
 std::printf("CATALOG faction=%d count=%d\n",int(faction),total);Check("both factions full normal catalog tested",total>40);
}
static int Cadence(const std::string& dir,StructType type,bool player){
 Check("load combat fixture",Load_Game((dir+"/fixture.sav").c_str()));Ready();Clear();
 auto good=HouseClass::As_Pointer(HOUSE_GOOD),bad=HouseClass::As_Pointer(HOUSE_BAD);PlayerPtr=good;good->IsHuman=true;bad->IsHuman=false;good->Power=bad->Power=1000000;bad->ROFBias=good->ROFBias;
 auto defense=new BuildingClass(type,player?HOUSE_GOOD:HOUSE_BAD);
 TechnoClass* target=type==STRUCT_SAM?static_cast<TechnoClass*>(new AircraftClass(AIRCRAFT_ORCA,player?HOUSE_BAD:HOUSE_GOOD)):static_cast<TechnoClass*>(new UnitClass(UNIT_MTANK,player?HOUSE_BAD:HOUSE_GOOD));
 bool placed=false;for(int y=Map.MapCellY+12;y<Map.MapCellY+Map.MapCellHeight-4&&!placed;++y)for(int x=Map.MapCellX+4;x<Map.MapCellX+Map.MapCellWidth-4;++x)if(defense->Can_Enter_Cell(XY_Cell(x,y))==MOVE_OK&&Map[XY_Cell(x,y-3)].Is_Generally_Clear()&&target->Can_Enter_Cell(XY_Cell(x,y-3))==MOVE_OK&&defense->Unlimbo(Cell_Coord(XY_Cell(x,y)),DIR_N)){placed=target->Unlimbo(Cell_Coord(XY_Cell(x,y-3)),DIR_N);break;}
 Check("place stationary defense and target",placed);if(!placed)return 0;
 ++ScenarioInit;defense->Begin_Mode(BSTATE_IDLE);--ScenarioInit;
 if(type==STRUCT_SAM)static_cast<AircraftClass*>(target)->Altitude=AircraftClass::FLIGHT_LEVEL;
 target->Strength=30000;target->IsDiscoveredByPlayer=true;defense->Assign_Target(target->As_Target());defense->Assign_Mission(MISSION_ATTACK);defense->Commence();std::vector<int> shots;
 for(int tick=0;tick<20000&&shots.size()<34;++tick){++Frame;int before=Bullets.Count();defense->AI();if(Bullets.Count()>before)shots.push_back(tick);for(int i=Bullets.Count()-1;i>=0;--i)delete Bullets.Ptr(i);}
 Check("sustained defense actually fires thirty-four shots",shots.size()==34);int duration=shots.size()==34?shots[33]-shots[3]:0;
 std::printf("CADENCE type=%d player=%d duration=%d rearm=%d,%d\n",int(type),int(player),duration,defense->Rearm_Delay(false),defense->Rearm_Delay(true));return duration;
}
bool Generate_Mission_Starts(const char* directory){std::string dir(directory);Production(dir,HOUSE_GOOD);Production(dir,HOUSE_BAD);
 for(auto t:{STRUCT_OBELISK,STRUCT_ATOWER,STRUCT_SAM}){int old=Cadence(dir,t,false),now=Cadence(dir,t,true);double factor=t==STRUCT_SAM?4:2;Check("player sustained attack rate doubles over previous profile within integer tick rounding",now>0&&double(old)/now>factor*0.96&&double(old)/now<factor*1.04);}
 Check("load no-base visual fixture",Load_Game((dir+"/fixture.sav").c_str()));Ready();Clear();PlayerPtr=HouseClass::As_Pointer(HOUSE_GOOD);PlayerPtr->IsHuman=true;Frame=100;PlayerPtr->Credits=50000;auto soldier=new InfantryClass(INFANTRY_E1,HOUSE_GOOD);Check("place preview soldier",Put(soldier));PlayerPtr->IsRecalcNeeded=true;PlayerPtr->AI();Map.Activate(1);for(int i=0;i<MAP_CELL_TOTAL;++i)if(Map.In_Radar(i))Map.Map_Cell(i,PlayerPtr,true);++ScenarioInit;Map.Set_Tactical_Position(Cell_Coord(XY_Cell(Cell_X(Coord_Cell(soldier->Coord))-2,Cell_Y(Coord_Cell(soldier->Coord))-2)));--ScenarioInit;
 Check("save no-MCV all-production preview",Save_Game((dir+"/savegame.093").c_str(),"NO MCV FULL PRODUCTION"));
 std::printf("TOTAL FAILURES %d\n",failures);return failures==0;}
