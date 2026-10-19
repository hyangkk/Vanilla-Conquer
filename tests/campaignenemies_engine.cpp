// Offline native campaign/production/save-compatibility probe. Never ship this entry point.
#include "function.h"
#include "campaignenemies.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
static int failures;
static void Check(const char* label,bool ok){std::printf("%s %s\n",ok?"PASS":"FAIL",label);std::fflush(stdout);if(!ok)++failures;}
static std::vector<FootClass*> Army(){std::vector<FootClass*> a;for(int i=0;i<Units.Count();++i)a.push_back(Units.Ptr(i));for(int i=0;i<Infantry.Count();++i)a.push_back(Infantry.Ptr(i));for(int i=0;i<Aircraft.Count();++i)a.push_back(Aircraft.Ptr(i));return a;}
static bool Hostile(HouseClass* house){return house!=PlayerPtr&&!house->IsHuman&&house->Class->House!=HOUSE_NEUTRAL&&!PlayerPtr->Is_Ally(house)&&!house->Is_Ally(PlayerPtr);}
static std::vector<int> Counts(){std::vector<int> v(HOUSE_COUNT*384+HOUSE_COUNT);for(auto f:Army())if(f->IsActive&&!f->IsInLimbo&&Map.In_Radar(Coord_Cell(f->Coord))){int k=f->What_Am_I()==RTTI_UNIT?0:f->What_Am_I()==RTTI_INFANTRY?1:2;int type=k==0?int(static_cast<UnitClass*>(f)->Class->Type):k==1?int(static_cast<InfantryClass*>(f)->Class->Type):int(static_cast<AircraftClass*>(f)->Class->Type);++v[int(f->House->Class->House)*384+k*128+type];}for(int i=0;i<Buildings.Count();++i){auto b=Buildings.Ptr(i);if(!b->IsInLimbo)++v[HOUSE_COUNT*384+int(b->House->Class->House)];}return v;}
static bool Place(FootClass* f,int seed=0){for(int y=Map.MapCellY+5;y<Map.MapCellY+Map.MapCellHeight-5;++y)for(int x=Map.MapCellX+5+seed;x<Map.MapCellX+Map.MapCellWidth-5;++x)if(f->Can_Enter_Cell(XY_Cell(x,y))==MOVE_OK&&f->Unlimbo(Cell_Coord(XY_Cell(x,y)),DIR_S)){f->Assign_Mission(MISSION_GUARD);f->Commence();return true;}std::printf("PLACE FAILED type=%d active=%d strength=%d map=%d,%d,%d,%d scenarioinit=%d\n",int(f->What_Am_I()),f->IsActive,f->Strength,Map.MapCellX,Map.MapCellY,Map.MapCellWidth,Map.MapCellHeight,ScenarioInit);return false;}
static int EnemyCount(){int n=0;for(auto f:Army())if(!f->IsInLimbo&&f->IsActive&&Hostile(f->House))++n;return n;}
bool Generate_Mission_Starts(const char* directory){
 GameToPlay=GAME_NORMAL;GameActive=true;PlayerWins=PlayerLoses=false;
 std::string dir(directory);bool baseline=std::getenv("VANILLA_ENEMY_BASELINE")!=nullptr;
 std::ofstream output;if(baseline)output.open(dir+"/baseline.txt");std::ifstream input;if(!baseline)input.open(dir+"/baseline.txt");
 for(int slot=100;slot<=224;++slot){if(slot>155&&slot<200)continue;char file[64];std::snprintf(file,sizeof(file),"/starts/savegame.%03d",slot);
  Check("load campaign start",Load_Game((dir+file).c_str()));auto counts=Counts();
  if(baseline){output<<slot;for(int n:counts)output<<' '<<n;output<<'\n';continue;}
  int oldslot;input>>oldslot;Check("baseline slot matches",oldslot==slot);bool exact=true;
  for(unsigned i=0;i<counts.size();++i){int n=-1;input>>n;int house=i<HOUSE_COUNT*384?i/384:i-HOUSE_COUNT*384;int multiplier=i<HOUSE_COUNT*384&&Hostile(HouseClass::As_Pointer(HousesType(house)))?2:1;if(counts[i]!=n*multiplier){std::printf("MISMATCH slot=%d index=%u original=%d actual=%d expected=%d\n",slot,i,n,counts[i],n*multiplier);exact=false;}}
  Check("every hostile type doubled; other units and buildings unchanged",exact);
  auto stable=Counts();Campaign_Enemies_Apply();Campaign_Enemies_Apply();Check("repeat apply does not compound",Counts()==stable);
  Check("save campaign doubled army",Save_Game((dir+"/roundtrip.sav").c_str(),"ENEMY ROUNDTRIP"));
  Check("reload campaign doubled army",Load_Game((dir+"/roundtrip.sav").c_str()));Check("save reload does not compound",Counts()==stable);
 }
 if(baseline)return true;
 Check("load isolated native fixture",Load_Game((dir+"/fixture.sav").c_str()));
 for(int i=0;i<Triggers.Count();++i)Triggers.Ptr(i)->Action=TriggerClass::ACTION_NONE;
 for(auto f:Army())f->Limbo();for(int i=Buildings.Count()-1;i>=0;--i)Buildings.Ptr(i)->Limbo();Campaign_Enemies_Reset();
 auto good=HouseClass::As_Pointer(HOUSE_GOOD),bad=HouseClass::As_Pointer(HOUSE_BAD),neutral=HouseClass::As_Pointer(HOUSE_NEUTRAL),ally=HouseClass::As_Pointer(HOUSE_JP);
 PlayerPtr=good;good->IsHuman=true;bad->IsHuman=false;bad->Make_Enemy(HOUSE_GOOD);good->Make_Enemy(HOUSE_BAD);good->Make_Ally(HOUSE_JP);ally->Make_Ally(HOUSE_GOOD);ally->IsHuman=false;GameToPlay=GAME_NORMAL;ScenarioInit=0;
 auto tank=new UnitClass(UNIT_MTANK,HOUSE_BAD);auto infantry=new InfantryClass(INFANTRY_E1,HOUSE_BAD);auto air=new AircraftClass(AIRCRAFT_ORCA,HOUSE_BAD);
 auto own=new UnitClass(UNIT_MTANK,HOUSE_GOOD);auto friendUnit=new UnitClass(UNIT_MTANK,HOUSE_JP);auto civilian=new InfantryClass(INFANTRY_C1,HOUSE_NEUTRAL);
 Check("place test combatants",Place(tank)&&Place(infantry,3)&&Place(air,8)&&Place(own,12)&&Place(friendUnit,16)&&Place(civilian,20));
 tank->Strength=211;tank->Ammo=2;
 // Save an empty bookkeeping footer, then trim it to reproduce a genuinely old native save.
 std::string legacy=dir+"/legacy.sav";Check("save undoubled fixture",Save_Game(legacy.c_str(),"OLD UNDOUBLED ARMY"));
 {std::ifstream in(legacy,std::ios::binary);std::vector<char> data((std::istreambuf_iterator<char>(in)),{});in.close();const std::string marker="CNCENM01";auto footer=std::search(data.begin(),data.end(),marker.begin(),marker.end());Check("empty footer exists",footer!=data.end());data.resize(footer-data.begin());std::ofstream out(legacy,std::ios::binary|std::ios::trunc);out.write(data.data(),data.size());}
 Check("legacy native save loads",Load_Game(legacy.c_str()));Check("old save doubles enemy once",EnemyCount()==6);
 FootClass* source=nullptr;FootClass* companion=nullptr;for(auto f:Army())if(!f->IsInLimbo&&f->What_Am_I()==RTTI_UNIT&&f->House==HouseClass::As_Pointer(HOUSE_BAD)){if(!source)source=f;else companion=f;}
 Check("cloned tank preserves HP and ammo",source&&companion&&source->Strength==211&&companion->Strength==211&&source->Ammo==2&&companion->Ammo==2);
 if(source&&companion){CELL cell=Coord_Cell(source->Coord);source->Limbo();Check("transport-style reappearance",source->Unlimbo(Cell_Coord(cell),DIR_N));Campaign_Enemies_Apply();Check("unload does not duplicate again",EnemyCount()==6);
  source->Assign_Destination(::As_Target(XY_Cell(Map.MapCellX+30,Map.MapCellY+20)));Campaign_Enemies_Apply();Check("companion follows updated movement orders",companion->NavCom==source->NavCom);
 }
 auto fresh=new UnitClass(UNIT_LTANK,HOUSE_BAD);Check("place newly produced enemy",Place(fresh,25));Campaign_Enemies_Apply();Check("future production or reinforcement doubles",EnemyCount()==8);
 auto skirmish=new UnitClass(UNIT_LTANK,HOUSE_BAD);Check("place skirmish enemy",Place(skirmish,28));GameToPlay=GAME_SKIRMISH;Campaign_Enemies_Apply();Check("skirmish gets no additional unit",EnemyCount()==9);GameToPlay=GAME_NORMAL;Campaign_Enemies_Apply();Check("campaign resumes doubling",EnemyCount()==10);
 int beforeReuse=EnemyCount();TARGET oldId=fresh->As_Target();delete fresh;
 auto reused=new UnitClass(UNIT_LTANK,HOUSE_BAD);Check("native heap reuses deleted actor slot",reused->As_Target()==oldId);Check("place reused slot",Place(reused,25));Campaign_Enemies_Apply();Check("new actor in reused slot is doubled once",EnemyCount()==beforeReuse+1);
 auto before=Counts();for(int i=0;i<3;++i){Check("save progress",Save_Game((dir+"/progress.sav").c_str(),"DOUBLE ENEMY PROGRESS"));Check("reload progress",Load_Game((dir+"/progress.sav").c_str()));Check("three repeated reloads remain exactly doubled",Counts()==before);}
 // Isolated visible preview; never touches live user saves.
 for(int i=0;i<MAP_CELL_TOTAL;++i)if(Map.In_Radar(i))Map.Map_Cell(i,PlayerPtr,true);
 ++ScenarioInit;Map.Set_Tactical_Position(Cell_Coord(XY_Cell(Map.MapCellX+2,Map.MapCellY+1)));--ScenarioInit;
 for(auto f:Army())if(!f->IsInLimbo){f->Assign_Target(TARGET_NONE);f->Assign_Mission(MISSION_SLEEP);f->Commence();}
 PlayerPtr->Credits=50000;PlayerPtr->IsToWin=PlayerPtr->IsToLose=PlayerPtr->IsToDie=false;PlayerWins=PlayerLoses=false;
 Check("save isolated doubled army preview",Save_Game((dir+"/savegame.097").c_str(),"DOUBLE ENEMY PREVIEW"));
 std::printf("TOTAL FAILURES %d\n",failures);return failures==0;
}
