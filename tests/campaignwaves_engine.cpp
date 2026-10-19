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
 auto before=Active();Campaign_Waves_Reset();int start=Frame;Campaign_Waves_AI();Frame+=90*TICKS_PER_SECOND;Campaign_Waves_AI();Check("pre-landing game defers additional enemy waves",Active()==before);Frame=start;Campaign_Waves_Reset();
 for(int i=0;i<Triggers.Count();++i){auto t=Triggers.Ptr(i);if(t->Action!=TriggerClass::ACTION_REINFORCEMENTS&&t->Action!=TriggerClass::ACTION_CREATE_TEAM&&t->Action!=TriggerClass::ACTION_DZ)t->Action=TriggerClass::ACTION_NONE;}
 for(int tick=0;tick<2*TICKS_PER_MINUTE&&!GroundPlayer();++tick){++Frame;PlayerWins=PlayerLoses=false;PlayerPtr->IsToWin=PlayerPtr->IsToLose=PlayerPtr->IsToDie=false;Logic.AI();}
 Check("native landing deploys a player ground force",GroundPlayer());
}
static FootClass* PlaceProbe(FootClass* f){
 for(int y=Map.MapCellY+3;y<Map.MapCellY+Map.MapCellHeight-3;++y)for(int x=Map.MapCellX+3;x<Map.MapCellX+Map.MapCellWidth-3;++x)if(f->Can_Enter_Cell(XY_Cell(x,y))==MOVE_OK&&f->Unlimbo(Cell_Coord(XY_Cell(x,y)),DIR_S)){f->Assign_Mission(MISSION_GUARD);f->Commence();return f;}return nullptr;
}
static CELL NewEntry(const std::set<TARGET>& before){for(auto f:Army())if(f->IsActive&&!f->IsInLimbo&&!before.count(f->As_Target()))return Coord_Cell(f->Coord);return 0;}
static void Aggression(const std::string& dir){
 Check("load aggression fixture",Load_Game((dir+"/starts/savegame.200").c_str()));Ready();
 for(auto f:Army())f->Limbo();for(int i=0;i<Buildings.Count();++i)Buildings.Ptr(i)->Limbo();Campaign_Waves_Reset();
 auto good=HouseClass::As_Pointer(HOUSE_GOOD),bad=HouseClass::As_Pointer(HOUSE_BAD),ally=HouseClass::As_Pointer(HOUSE_JP);PlayerPtr=good;good->IsHuman=true;bad->IsHuman=false;good->Make_Enemy(HOUSE_BAD);bad->Make_Enemy(HOUSE_GOOD);good->Make_Ally(HOUSE_JP);ally->Make_Ally(HOUSE_GOOD);
 auto own=PlaceProbe(new UnitClass(UNIT_MTANK,HOUSE_GOOD));std::vector<FootClass*> guards;
 for(int i=0;i<12;++i)guards.push_back(PlaceProbe(new UnitClass(UNIT_MTANK,HOUSE_BAD)));
 auto harvester=PlaceProbe(new UnitClass(UNIT_HARVESTER,HOUSE_BAD));auto transport=PlaceProbe(new UnitClass(UNIT_APC,HOUSE_BAD));auto engineer=PlaceProbe(new InfantryClass(INFANTRY_E7,HOUSE_BAD));auto friendly=PlaceProbe(new UnitClass(UNIT_MTANK,HOUSE_JP));auto civilian=PlaceProbe(new InfantryClass(INFANTRY_C1,HOUSE_NEUTRAL));auto scripted=PlaceProbe(new UnitClass(UNIT_MTANK,HOUSE_BAD));
 auto tt=new TeamTypeClass();tt->House=HOUSE_BAD;tt->ClassCount=1;tt->Class[0]=&UnitTypeClass::As_Reference(UNIT_MTANK);tt->DesiredNum[0]=1;tt->MissionCount=1;tt->MissionList[0].Mission=TMISSION_GUARD;tt->MissionList[0].Argument=100;
 auto team=new TeamClass(tt,bad);Check("scripted team contains protected guard",team->Add(scripted));
 // Diagnostic guards represent a final population, already handled by the multiplier.
 for(auto f:Army())if(!f->IsInLimbo&&f->House==bad)Campaign_Enemies_Mark_Extra(f);
 auto triggerGuard=PlaceProbe(new UnitClass(UNIT_MTANK,HOUSE_BAD));auto actorTrigger=new TriggerClass();actorTrigger->Action=TriggerClass::ACTION_NONE;triggerGuard->Trigger=actorTrigger;++actorTrigger->AttachCount;Campaign_Enemies_Mark_Extra(triggerGuard);
 auto original=Active();auto hunts=[&](){int n=0;for(auto f:guards)if(f&&f->Mission==MISSION_HUNT)++n;return n;};
 Frame=0;Campaign_Waves_AI();Frame=30*TICKS_PER_SECOND;Campaign_Waves_AI();Check("opening mobilizes one quarter of twelve guards",hunts()==3);
 std::vector<FootClass*> attackers;std::vector<COORDINATE> positions;for(auto f:guards)if(f->Mission==MISSION_HUNT){attackers.push_back(f);positions.push_back(f->Coord);}own->Strength=20000;
 bool fired=false;for(int tick=0;tick<400;++tick){++Frame;for(auto f:attackers)if(f->IsActive&&!f->IsInLimbo)f->AI();if(Bullets.Count())fired=true;for(int i=Bullets.Count()-1;i>=0;--i)if(Bullets.Ptr(i)->IsActive)Bullets.Ptr(i)->AI();}
 bool moved=false;for(unsigned i=0;i<attackers.size();++i)if(attackers[i]->Coord!=positions[i])moved=true;Check("mobilized old guards actually move",moved);Check("mobilized old guards actually fire",fired);

 Check("own allied neutral transport harvester engineer and scripted roles preserved",own->Mission==MISSION_GUARD&&friendly->Mission==MISSION_GUARD&&civilian->Mission==MISSION_GUARD&&harvester->Mission==MISSION_GUARD&&transport->Mission==MISSION_GUARD&&engineer->Mission==MISSION_GUARD&&scripted->Mission==MISSION_GUARD&&scripted->Team==team&&triggerGuard->Mission==MISSION_GUARD);
 std::vector<TARGET> initialNav;for(auto f:guards)initialNav.push_back(f->NavCom);Frame=40*TICKS_PER_SECOND;Campaign_Waves_AI();bool stable=true;for(unsigned i=0;i<guards.size();++i)if(initialNav[i]!=guards[i]->NavCom)stable=false;Check("already attacking units retain their paths",stable);
 auto deadline=[&](){CDFileClass file;file.Open((dir+"/clock.bin").c_str(),WRITE);Campaign_Waves_Save(file);file.Close();std::ifstream in(dir+"/clock.bin",std::ios::binary);in.seekg(8);int next=-1;in.read(reinterpret_cast<char*>(&next),sizeof(next));return next;};
 for(int round=1;round<=12;++round){int begin=deadline();Frame=begin;Campaign_Waves_AI();Check("next deadline uses exact escalating interval",deadline()-begin==(round>=12?30:90-round*5)*TICKS_PER_SECOND);while(true){auto snapshot=Active();Frame+=TICKS_PER_SECOND;Campaign_Waves_AI();if(Active()==snapshot)break;}
  int expected=round>=9?12:round>=6?9:round>=3?6:3;
  Frame=((Frame+10*TICKS_PER_SECOND-1)/(10*TICKS_PER_SECOND))*10*TICKS_PER_SECOND;Campaign_Waves_AI();Check("guard mobilization grows by saved assault phase",hunts()==expected);
  Check("wave interval progressively reaches thirty seconds",deadline()-Frame<=((round>=12?30:90-round*5)*TICKS_PER_SECOND));
  // Free only diagnostic reinforcements so late-stage checks reach their real deadline.
  for(auto f:Army())if(f->IsActive&&!f->IsInLimbo&&!original.count(f->As_Target()))delete f;
 }
 Check("late phase commits every eligible guard",hunts()==12);
 auto before=Active();Frame=deadline()-1;Campaign_Waves_AI();Check("late-stage wave does not arrive before thirty-second deadline",Active()==before);++Frame;Campaign_Waves_AI();
 for(int i=0;i<5;++i){Frame+=TICKS_PER_SECOND;Campaign_Waves_AI();}
 Check("thirteenth wave brings eighty-four final units",Active().size()==before.size()+84);
 auto saved=Active();Check("save aggressive phase",Save_Game((dir+"/pressure-progress.sav").c_str(),"PRESSURE PHASE 13"));Check("reload aggressive phase",Load_Game((dir+"/pressure-progress.sav").c_str()));Ready();Campaign_Waves_AI();Check("aggressive phase reload does not replay the wave",Active()==saved);
 // Prior-format partial waves keep their remaining count under the new profile.
 Check("load previous ten-unit partial wave",Load_Game((dir+"/old-partial-wave.sav").c_str()));Ready();auto old=Active();Campaign_Waves_AI();Check("previous partial wave adds only its remaining two",Active().size()==old.size()+2);
}
bool Generate_Mission_Starts(const char* directory){
 std::string dir(directory);GameToPlay=GAME_NORMAL;GameActive=true;int examined=0;
 for(int slot=100;slot<=224;++slot){if(slot>155&&slot<200)continue;char name[64];std::snprintf(name,sizeof(name),"/starts/savegame.%03d",slot);
  Check("load original campaign start",Load_Game((dir+name).c_str()));LandPlayer();Ready();Campaign_Waves_Reset();Frame=0;
  auto initial=Active();int own=Own();Campaign_Waves_AI();Frame=90*TICKS_PER_SECOND-1;Campaign_Waves_AI();Check("no reinforcements before ninety seconds",Active()==initial);
  Frame=90*TICKS_PER_SECOND;Campaign_Waves_AI();auto first=Active();CELL firstEntry=NewEntry(initial);
  std::printf("SLOT %d base=%zu first=%zu\n",slot,initial.size(),first.size());
  Check("first wave delivers exactly twelve units",first.size()==initial.size()+12);if(first.size()!=initial.size()+12){
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
  Frame=175*TICKS_PER_SECOND-1;Campaign_Waves_AI();Check("second wave does not arrive early",Active()==first);
  Check("save one tick before second wave",Save_Game((dir+"/before-wave.sav").c_str(),"WAVE DEADLINE CHECK"));Check("reload wave deadline",Load_Game((dir+"/before-wave.sav").c_str()));Ready();Campaign_Waves_AI();Check("reload preserves remaining one-tick countdown",Active()==first);
  ++Frame;Campaign_Waves_AI();auto second=Active();CELL secondEntry=NewEntry(first);if(slot==200)Check("second assault changes approach direction",firstEntry!=secondEntry);Check("second wave grows to eighteen",second.size()==initial.size()+30);
  Check("save immediately after wave",Save_Game((dir+"/after-wave.sav").c_str(),"WAVE COUNT CHECK"));Check("reload immediately after wave",Load_Game((dir+"/after-wave.sav").c_str()));Ready();Campaign_Waves_AI();Check("reload does not replay delivered wave",Active()==second);
  Frame=255*TICKS_PER_SECOND;Campaign_Waves_AI();
  for(int retry=0;retry<10&&Active().size()<initial.size()+54;++retry){Frame+=TICKS_PER_SECOND;Campaign_Waves_AI();}
  Check("third wave grows to twenty-four",Active().size()==initial.size()+54);if(slot==200)Check("third assault changes approach direction again",NewEntry(second)!=secondEntry);
  if(slot==200){
   Frame=330*TICKS_PER_SECOND;Campaign_Waves_AI();auto partial=Active();Check("fourth wave starts with bounded twenty-four-unit batch",partial.size()==initial.size()+78);
   Check("save partial arrival",Save_Game((dir+"/partial-wave.sav").c_str(),"PARTIAL WAVE"));Check("reload partial arrival",Load_Game((dir+"/partial-wave.sav").c_str()));Ready();Check("partial reload does not recreate twenty-four arrived units",Active()==partial);
   Frame+=TICKS_PER_SECOND;Campaign_Waves_AI();Check("fourth wave resumes only remaining six units",Active().size()==initial.size()+84);
  }
 }
 Check("all eighty-one starts reinforced",examined==81);
 Aggression(dir);
 // Capture the old enemy footer without its new optional wave extension.
 Check("load old enemy-multiplier progress",Load_Game((dir+"/previous-progress.sav").c_str()));Ready();Campaign_Waves_Reset();auto old=Active();int start=Frame;Campaign_Waves_AI();Frame=start+90*TICKS_PER_SECOND-1;Campaign_Waves_AI();Check("old progress gets full ninety-second grace",Active()==old);++Frame;Campaign_Waves_AI();Check("old progress receives first twelve-unit wave",Active().size()==old.size()+12);
 // Native movement test and a visual fixture begin from Blackout's original start.
 Check("load movement fixture",Load_Game((dir+"/starts/savegame.200").c_str()));Ready();Campaign_Waves_Reset();Frame=0;Campaign_Waves_AI();auto baseline=Active();Frame=90*TICKS_PER_SECOND;Campaign_Waves_AI();
 std::vector<FootClass*> arrivals;for(auto f:Army())if(!f->IsInLimbo&&!baseline.count(f->As_Target()))arrivals.push_back(f);
 Check("movement fixture has twelve real arrivals",arrivals.size()==12);if(arrivals.empty())return false;
 COORDINATE goal=As_Techno(arrivals[0]->TarCom)->Center_Coord();CELL entry=Coord_Cell(arrivals[0]->Coord);
 for(int i=0;i<Buildings.Count();++i)if(!Buildings.Ptr(i)->IsInLimbo&&Buildings.Ptr(i)->House==PlayerPtr)Buildings.Ptr(i)->Strength=30000;
 std::vector<int> distances;for(auto f:arrivals)distances.push_back(Distance(Coord_Cell(f->Coord),Coord_Cell(goal)));
 for(int tick=0;tick<600;++tick){++Frame;for(auto f:arrivals)if(f->IsActive&&!f->IsInLimbo)f->AI();for(int i=Bullets.Count()-1;i>=0;--i)if(Bullets.Ptr(i)->IsActive)Bullets.Ptr(i)->AI();}
 bool approached=false;for(unsigned i=0;i<arrivals.size();++i)if(arrivals[i]->IsActive&&!arrivals[i]->IsInLimbo&&Distance(Coord_Cell(arrivals[i]->Coord),Coord_Cell(goal))+3<distances[i])approached=true;
 Check("native foot and vehicle AI actually approach the player",approached);
 // Existing neutral/allied-only and ended games must not gain hostile waves.
 auto count=Active();GameToPlay=GAME_SKIRMISH;Frame+=90*TICKS_PER_SECOND;Campaign_Waves_AI();Check("skirmish excludes periodic waves",Active()==count);GameToPlay=GAME_NORMAL;PlayerWins=true;Campaign_Waves_AI();Check("victory excludes periodic waves",Active()==count);PlayerWins=false;PlayerLoses=true;Campaign_Waves_AI();Check("defeat excludes periodic waves",Active()==count);PlayerLoses=false;
 Check("reload unfinished fourth wave",Load_Game((dir+"/partial-wave.sav").c_str()));Ready();
 for(auto f:Army())if(!f->IsInLimbo&&f->House!=PlayerPtr&&f->House->Class->House!=HOUSE_NEUTRAL)f->Limbo();
 for(int i=0;i<Buildings.Count();++i){auto b=Buildings.Ptr(i);if(!b->IsInLimbo&&b->House!=PlayerPtr&&b->House->Class->House!=HOUSE_NEUTRAL)b->Limbo();}
 count=Active();Frame+=TICKS_PER_SECOND;Campaign_Waves_AI();Check("unfinished arrival does not resurrect eliminated enemy",Active()==count);Campaign_Waves_Reset();Campaign_Waves_AI();Frame+=90*TICKS_PER_SECOND;Campaign_Waves_AI();Check("eliminated enemy faction is not resurrected",Active()==count);
 PlayerPtr->IsHuman=false;Campaign_Waves_AI();Check("AI-controlled local slot gets no waves",Active()==count);PlayerPtr->IsHuman=true;
 // Preview starts two game seconds before the first real arrival, with the camera at entry.
 Check("reload preview start",Load_Game((dir+"/starts/savegame.200").c_str()));Ready();Campaign_Waves_Reset();Frame=0;Campaign_Waves_AI();Frame=90*TICKS_PER_SECOND-2*TICKS_PER_SECOND;
 for(int i=0;i<MAP_CELL_TOTAL;++i)if(Map.In_Radar(i))Map.Map_Cell(i,PlayerPtr,true);
 ++ScenarioInit;Map.Set_Tactical_Position(Cell_Coord(XY_Cell(MAX(Map.MapCellX,Cell_X(entry)-5),MAX(Map.MapCellY,Cell_Y(entry)-5))));--ScenarioInit;
 PlayerPtr->Credits=50000;Check("save real arrival preview",Save_Game((dir+"/savegame.094").c_str(),"ESCALATING ASSAULT 12 UNITS"));
 std::printf("TOTAL FAILURES %d\n",failures);return failures==0;
}
