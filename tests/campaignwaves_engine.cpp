// Offline native reinforcement scheduler and movement probe. Never ship this entry point.
#include "function.h"
#include "campaignwaves.h"
#include "campaignenemies.h"
#include <cstdio>
#include <string>
#include <set>
#include <vector>
#include <fstream>
#include <algorithm>
static int failures;
static void Check(const char* name,bool ok){std::printf("%s %s\n",ok?"PASS":"FAIL",name);std::fflush(stdout);if(!ok)++failures;}
static std::vector<FootClass*> Army(){std::vector<FootClass*> a;for(int i=0;i<Units.Count();++i)a.push_back(Units.Ptr(i));for(int i=0;i<Infantry.Count();++i)a.push_back(Infantry.Ptr(i));for(int i=0;i<Aircraft.Count();++i)a.push_back(Aircraft.Ptr(i));return a;}
static std::set<TARGET> Active(){std::set<TARGET> a;for(auto f:Army())if(f->IsActive&&!f->IsInLimbo)a.insert(f->As_Target());return a;}
static void Ready(){GameToPlay=GAME_NORMAL;GameActive=true;PlayerWins=PlayerLoses=false;PlayerPtr->IsToWin=PlayerPtr->IsToLose=PlayerPtr->IsToDie=false;for(int i=0;i<Triggers.Count();++i)Triggers.Ptr(i)->Action=TriggerClass::ACTION_NONE;}
static int Own(){int n=0;for(auto f:Army())if(!f->IsInLimbo&&f->House==PlayerPtr)++n;return n;}
static bool GroundPlayer(){for(int i=0;i<Buildings.Count();++i)if(!Buildings.Ptr(i)->IsInLimbo&&Buildings.Ptr(i)->House==PlayerPtr)return true;for(auto f:Army())if(!f->IsInLimbo&&f->House==PlayerPtr&&f->What_Am_I()!=RTTI_AIRCRAFT&&Map.In_Radar(Coord_Cell(f->Coord)))return true;return false;}
static void LandPlayer(){
 if(GroundPlayer())return;
 auto before=Active();Campaign_Waves_Reset();int start=Frame;Campaign_Waves_AI();Frame+=3*TICKS_PER_MINUTE;Campaign_Waves_AI();Check("pre-landing game defers additional enemy waves",Active()==before);Frame=start;Campaign_Waves_Reset();
 for(int i=0;i<Triggers.Count();++i){auto t=Triggers.Ptr(i);if(t->Action!=TriggerClass::ACTION_REINFORCEMENTS&&t->Action!=TriggerClass::ACTION_CREATE_TEAM&&t->Action!=TriggerClass::ACTION_DZ)t->Action=TriggerClass::ACTION_NONE;}
 for(int tick=0;tick<2*TICKS_PER_MINUTE&&!GroundPlayer();++tick){++Frame;PlayerWins=PlayerLoses=false;PlayerPtr->IsToWin=PlayerPtr->IsToLose=PlayerPtr->IsToDie=false;Logic.AI();}
 Check("native landing deploys a player ground force",GroundPlayer());
}
bool Generate_Mission_Starts(const char* directory){
 std::string dir(directory);GameToPlay=GAME_NORMAL;GameActive=true;int examined=0;
 for(int slot=100;slot<=224;++slot){if(slot>155&&slot<200)continue;char name[64];std::snprintf(name,sizeof(name),"/starts/savegame.%03d",slot);
  Check("load original campaign start",Load_Game((dir+name).c_str()));LandPlayer();Ready();Campaign_Waves_Reset();Frame=0;
  auto initial=Active();int own=Own();Campaign_Waves_AI();Frame=3*TICKS_PER_MINUTE-1;Campaign_Waves_AI();Check("no reinforcements before three minutes",Active()==initial);
  Frame=3*TICKS_PER_MINUTE;Campaign_Waves_AI();auto first=Active();
  std::printf("SLOT %d base=%zu first=%zu\n",slot,initial.size(),first.size());
  Check("first wave delivers exactly six units",first.size()==initial.size()+6);if(first.size()!=initial.size()+6){
   Save_Game((dir+"/failed-"+std::to_string(slot)+".sav").c_str(),"WAVE DEBUG");
   std::printf("DEBUG slot=%d map=%d,%d,%d,%d player=%d\n",slot,Map.MapCellX,Map.MapCellY,Map.MapCellWidth,Map.MapCellHeight,int(PlayerPtr->Class->House));
   for(int h=0;h<HOUSE_COUNT;++h){auto house=HouseClass::As_Pointer(HousesType(h));int b=0,f=0;for(int i=0;i<Buildings.Count();++i)if(!Buildings.Ptr(i)->IsInLimbo&&Buildings.Ptr(i)->House==house)++b;for(auto u:Army())if(!u->IsInLimbo&&u->House==house)++f;if(b||f)std::printf("HOUSE %d human=%d defeated=%d ally=%d,%d buildings=%d army=%d\n",h,house->IsHuman,house->IsDefeated,house->Is_Ally(PlayerPtr),PlayerPtr->Is_Ally(house),b,f);}
   continue;
  }++examined;
  bool hostile=true,dinosaurOnly=true;for(auto f:Army())if(first.count(f->As_Target())&&!initial.count(f->As_Target())){
   hostile=hostile&&f->House!=PlayerPtr&&!f->House->Is_Ally(PlayerPtr)&&!PlayerPtr->Is_Ally(f->House)&&f->Mission==MISSION_HUNT;
   if(Special.IsJurassic)dinosaurOnly=dinosaurOnly&&f->What_Am_I()==RTTI_UNIT&&static_cast<UnitClass*>(f)->Class->Type>=UNIT_TRIC&&static_cast<UnitClass*>(f)->Class->Type<=UNIT_STEG;
  }
  Check("arrivals are hostile and hunt the player",hostile);if(Special.IsJurassic)Check("Jurassic reinforcements preserve dinosaurs",dinosaurOnly);
  Campaign_Enemies_Apply();Check("periodic wave is not doubled again",Active()==first);Check("player mobile army unchanged",Own()==own);
  Frame=6*TICKS_PER_MINUTE-1;Campaign_Waves_AI();Check("second wave does not arrive early",Active()==first);
  Check("save one tick before second wave",Save_Game((dir+"/before-wave.sav").c_str(),"WAVE DEADLINE CHECK"));Check("reload wave deadline",Load_Game((dir+"/before-wave.sav").c_str()));Ready();Campaign_Waves_AI();Check("reload preserves remaining one-tick countdown",Active()==first);
  ++Frame;Campaign_Waves_AI();auto second=Active();Check("second wave grows to eight",second.size()==initial.size()+14);
  Check("save immediately after wave",Save_Game((dir+"/after-wave.sav").c_str(),"WAVE COUNT CHECK"));Check("reload immediately after wave",Load_Game((dir+"/after-wave.sav").c_str()));Ready();Campaign_Waves_AI();Check("reload does not replay delivered wave",Active()==second);
  Frame=9*TICKS_PER_MINUTE;Campaign_Waves_AI();
  if(slot==200){
   auto partial=Active();Check("third wave starts with bounded eight-unit batch",partial.size()==initial.size()+22);
   Check("save partial arrival",Save_Game((dir+"/partial-wave.sav").c_str(),"PARTIAL WAVE"));Check("reload partial arrival",Load_Game((dir+"/partial-wave.sav").c_str()));Ready();Check("partial reload does not recreate eight arrived units",Active()==partial);
  }
  Frame+=TICKS_PER_SECOND;Campaign_Waves_AI();Check("third wave grows to ten across bounded batches",Active().size()==initial.size()+24);
 }
 Check("all eighty-one starts reinforced",examined==81);
 // Capture the old enemy footer without its new optional wave extension.
 Check("load old enemy-multiplier progress",Load_Game((dir+"/previous-progress.sav").c_str()));Ready();Campaign_Waves_Reset();auto old=Active();int start=Frame;Campaign_Waves_AI();Frame=start+3*TICKS_PER_MINUTE-1;Campaign_Waves_AI();Check("old progress gets full three-minute grace",Active()==old);++Frame;Campaign_Waves_AI();Check("old progress receives first six-unit wave",Active().size()==old.size()+6);
 // Native movement test and a visual fixture begin from Blackout's original start.
 Check("load movement fixture",Load_Game((dir+"/starts/savegame.200").c_str()));Ready();Campaign_Waves_Reset();Frame=0;Campaign_Waves_AI();auto baseline=Active();Frame=3*TICKS_PER_MINUTE;Campaign_Waves_AI();
 std::vector<FootClass*> arrivals;for(auto f:Army())if(!f->IsInLimbo&&!baseline.count(f->As_Target()))arrivals.push_back(f);
 Check("movement fixture has six real arrivals",arrivals.size()==6);if(arrivals.empty())return false;
 COORDINATE goal=As_Techno(arrivals[0]->TarCom)->Center_Coord();CELL entry=Coord_Cell(arrivals[0]->Coord);
 for(int i=0;i<Buildings.Count();++i)if(!Buildings.Ptr(i)->IsInLimbo&&Buildings.Ptr(i)->House==PlayerPtr)Buildings.Ptr(i)->Strength=30000;
 std::vector<int> distances;for(auto f:arrivals)distances.push_back(Distance(Coord_Cell(f->Coord),Coord_Cell(goal)));
 for(int tick=0;tick<600;++tick){++Frame;for(auto f:arrivals)if(f->IsActive&&!f->IsInLimbo)f->AI();for(int i=Bullets.Count()-1;i>=0;--i)if(Bullets.Ptr(i)->IsActive)Bullets.Ptr(i)->AI();}
 bool approached=false;for(unsigned i=0;i<arrivals.size();++i)if(arrivals[i]->IsActive&&!arrivals[i]->IsInLimbo&&Distance(Coord_Cell(arrivals[i]->Coord),Coord_Cell(goal))+3<distances[i])approached=true;
 Check("native foot and vehicle AI actually approach the player",approached);
 // Existing neutral/allied-only and ended games must not gain hostile waves.
 auto count=Active();GameToPlay=GAME_SKIRMISH;Frame+=3*TICKS_PER_MINUTE;Campaign_Waves_AI();Check("skirmish excludes periodic waves",Active()==count);GameToPlay=GAME_NORMAL;PlayerWins=true;Campaign_Waves_AI();Check("victory excludes periodic waves",Active()==count);PlayerWins=false;PlayerLoses=true;Campaign_Waves_AI();Check("defeat excludes periodic waves",Active()==count);PlayerLoses=false;
 Check("reload unfinished third wave",Load_Game((dir+"/partial-wave.sav").c_str()));Ready();
 for(auto f:Army())if(!f->IsInLimbo&&f->House!=PlayerPtr&&f->House->Class->House!=HOUSE_NEUTRAL)f->Limbo();
 for(int i=0;i<Buildings.Count();++i){auto b=Buildings.Ptr(i);if(!b->IsInLimbo&&b->House!=PlayerPtr&&b->House->Class->House!=HOUSE_NEUTRAL)b->Limbo();}
 count=Active();Frame+=TICKS_PER_SECOND;Campaign_Waves_AI();Check("unfinished arrival does not resurrect eliminated enemy",Active()==count);Campaign_Waves_Reset();Campaign_Waves_AI();Frame+=3*TICKS_PER_MINUTE;Campaign_Waves_AI();Check("eliminated enemy faction is not resurrected",Active()==count);
 PlayerPtr->IsHuman=false;Campaign_Waves_AI();Check("AI-controlled local slot gets no waves",Active()==count);PlayerPtr->IsHuman=true;
 // Preview starts two game seconds before the first real arrival, with the camera at entry.
 Check("reload preview start",Load_Game((dir+"/starts/savegame.200").c_str()));Ready();Campaign_Waves_Reset();Frame=0;Campaign_Waves_AI();Frame=3*TICKS_PER_MINUTE-2*TICKS_PER_SECOND;
 for(int i=0;i<MAP_CELL_TOTAL;++i)if(Map.In_Radar(i))Map.Map_Cell(i,PlayerPtr,true);
 ++ScenarioInit;Map.Set_Tactical_Position(Cell_Coord(XY_Cell(MAX(Map.MapCellX,Cell_X(entry)-5),MAX(Map.MapCellY,Cell_Y(entry)-5))));--ScenarioInit;
 PlayerPtr->Credits=50000;Check("save real arrival preview",Save_Game((dir+"/savegame.099").c_str(),"GROWING ENEMY WAVE PREVIEW"));
 std::printf("TOTAL FAILURES %d\n",failures);return failures==0;
}
