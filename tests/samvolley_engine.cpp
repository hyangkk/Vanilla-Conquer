// Private native integration: real save loader, SAM state machine, bullet physics and renderer.
#include "../tests/samtest_support.h"
#include "common/filepcx.h"
static BuildingClass* SV_Ready(const std::string& dir,bool player=true){
 Reset(dir);auto sam=new BuildingClass(STRUCT_SAM,player?HOUSE_GOOD:HOUSE_BAD);Check("place SAM in actual mission terrain",Put(sam,Center()));
 ++ScenarioInit;sam->Begin_Mode(BSTATE_IDLE);--ScenarioInit;sam->Assign_Mission(MISSION_GUARD);sam->Commence();return sam;
}
static std::vector<TARGET> SV_Enemies(BuildingClass* sam,int wanted){
 std::vector<TARGET> ids;auto c=Coord_Cell(sam->Coord);
 for(int n=0;n<wanted;++n){TechnoClass* unit=n<450?static_cast<TechnoClass*>(new InfantryClass(INFANTRY_E1,HOUSE_BAD)):static_cast<TechnoClass*>(new UnitClass(UNIT_MTANK,HOUSE_BAD));
  bool placed=false;
  for(int radius=3;radius<=16&&!placed;++radius)for(int dy=-radius;dy<=radius&&!placed;++dy)for(int dx=-radius;dx<=radius;++dx){
   if(MAX(ABS(dx),ABS(dy))!=radius)continue;int x=Cell_X(c)+dx,y=Cell_Y(c)+dy;if(x<0||x>=MAP_CELL_W||y<0||y>=MAP_CELL_H)continue;
   CELL cell=XY_Cell(x,y);if(!Map.In_Radar(cell)||::Distance(sam->Fire_Coord(0),Cell_Coord(cell))>18*ICON_LEPTON_W||unit->Can_Enter_Cell(cell)!=MOVE_OK)continue;
   if(unit->Unlimbo(Cell_Coord(cell),DIR_N)){placed=true;break;}
  }
  Check("place hostile within twenty-cell firing range",placed);if(!placed){delete unit;break;}
  unit->Strength=30000;unit->IsDiscoveredByPlayer=true;unit->Assign_Mission(MISSION_SLEEP);unit->Commence();Campaign_Enemies_Mark_Extra(unit);ids.push_back(unit->As_Target());
 }
 Check("all requested field enemies exist",int(ids.size())==wanted);return ids;
}
static void SV_Density(const std::string& dir){
 struct Row{int enemies,shots;};Row rows[]={{0,1},{9,1},{10,2},{19,2},{20,3},{39,3},{40,4},{79,4},{80,5},{159,5},{160,6},{319,6},{320,7},{639,7},{640,8}};
 for(auto row:rows){auto sam=SV_Ready(dir);auto ids=SV_Enemies(sam,row.enemies);Check("twenty-cell stock-player range",sam->Weapon_Range(0)==20*ICON_LEPTON_W);
  Check("doubling population threshold determines simultaneous shots",Campaign_SAM_Density_Shots(sam)==row.shots);
  if(ids.empty())continue;sam->Assign_Target(ids.front());auto root=sam->TechnoClass::Fire_At(ids.front(),0);
  Check("real fire creates exact simultaneous root missile count",root&&Bullets.Count()==row.shots);
  bool powers=true;for(int i=0;i<Bullets.Count();++i)powers=powers&&Bullets.Ptr(i)->Strength==50;
  Check("every simultaneous missile retains current fifty damage",powers);
  auto id=sam->As_Target();Check("save simultaneous live volley",Save_Game((dir+"/volley.sav").c_str(),"SAM simultaneous volley"));Check("reload simultaneous live volley",Load_Game((dir+"/volley.sav").c_str()));
  Check("reload never fires additional roots",Bullets.Count()==row.shots);sam=static_cast<BuildingClass*>(As_Techno(id));
  for(auto target:ids){auto o=As_Techno(target);if(o)o->Limbo();}
  Check("density recomputes after enemies leave",Campaign_SAM_Density_Shots(sam)==1);
 }
}
static void SV_RangeAndScope(const std::string& dir){
 auto sam=SV_Ready(dir);auto ids=SV_Enemies(sam,9);auto edge=new UnitClass(UNIT_MTANK,HOUSE_BAD);Put(edge,Center());edge->IsDiscoveredByPlayer=true;edge->Strength=30000;Campaign_Enemies_Mark_Extra(edge);
 auto fire=sam->Fire_Coord(0);edge->Coord=XY_Coord(Coord_X(fire)+20*ICON_LEPTON_W,Coord_Y(fire));
 Check("twenty-cell boundary is in range",sam->In_Range(edge->Target_Coord(),0)&&Campaign_SAM_Density_Shots(sam)==2);
 edge->Coord=XY_Coord(Coord_X(fire)+20*ICON_LEPTON_W+1,Coord_Y(fire));Check("twenty cells plus one lepton is outside range",!sam->In_Range(edge->Target_Coord(),0)&&Campaign_SAM_Density_Shots(sam)==1);
 edge->Coord=Cell_Coord(Center());edge->IsDiscoveredByPlayer=false;Check("hidden unit does not create extra attacks",Campaign_SAM_Density_Shots(sam)==1);edge->IsDiscoveredByPlayer=true;edge->Cloak=CLOAKED;Check("cloaked actor is excluded",Campaign_SAM_Density_Shots(sam)==1);edge->Cloak=UNCLOAKED;edge->Strength=0;Check("dead actor is excluded",Campaign_SAM_Density_Shots(sam)==1);
 auto ally=new UnitClass(UNIT_MTANK,HOUSE_GOOD);Put(ally,Center());ally->IsDiscoveredByPlayer=true;auto neutral=new UnitClass(UNIT_MTANK,HOUSE_NEUTRAL);Put(neutral,Center());neutral->IsDiscoveredByPlayer=true;auto building=new BuildingClass(STRUCT_POWER,HOUSE_BAD);Put(building,XY_Cell(Cell_X(Center())+8,Cell_Y(Center())+8));building->IsDiscoveredByPlayer=true;
 Check("own neutral and building actors do not increase troop density",Campaign_SAM_Density_Shots(sam)==1);
 sam=SV_Ready(dir,false);auto target=new AircraftClass(AIRCRAFT_ORCA,HOUSE_GOOD);Put(target,Center());target->Altitude=AircraftClass::FLIGHT_LEVEL;target->IsDiscoveredByPlayer=true;sam->Assign_Target(target->As_Target());
 auto shot=sam->TechnoClass::Fire_At(target->As_Target(),0);Check("enemy SAM range retains prior campaign setting",sam->Weapon_Range(0)==2*Weapons[WEAPON_NIKE].Range);Check("enemy SAM never gains player volley or chain",shot&&Bullets.Count()==1&&!Campaign_SAM_Chain_Impact(shot,target));
 sam=SV_Ready(dir);GameToPlay=GAME_SKIRMISH;Check("skirmish retains stock weapon range",sam->Weapon_Range(0)==Weapons[WEAPON_NIKE].Range);GameToPlay=GAME_NORMAL;
}
static void SV_Retarget(const std::string& dir){
 auto sam=SV_Ready(dir);auto center=Coord_Cell(sam->Coord);auto target=new UnitClass(UNIT_MTANK,HOUSE_BAD);
 Check("place former long-range target",Put(target,XY_Cell(Cell_X(center)+25,Cell_Y(center))));target->Strength=30000;target->IsDiscoveredByPlayer=true;Campaign_Enemies_Mark_Extra(target);
 Check("target begins beyond new twenty-cell range",!sam->In_Range(target->Target_Coord(),0));sam->Assign_Target(target->As_Target());sam->Assign_Mission(MISSION_ATTACK);sam->Commence();
 for(int i=0;i<200;++i){++Frame;Logic.AI();}
 Check("actual SAM AI never fires at former out-of-range target",Bullets.Count()==0);
 target->Limbo();bool placed=false;auto preferred=XY_Cell(Cell_X(center)+19,Cell_Y(center));
 for(int radius=0;radius<12&&!placed;++radius)for(int dy=-radius;dy<=radius&&!placed;++dy)for(int dx=-radius;dx<=radius;++dx){
  if(MAX(ABS(dx),ABS(dy))!=radius)continue;int x=Cell_X(preferred)+dx,y=Cell_Y(preferred)+dy;if(x<0||x>=MAP_CELL_W||y<0||y>=MAP_CELL_H)continue;auto cell=XY_Cell(x,y);
  if(!Map.In_Radar(cell)||target->Can_Enter_Cell(cell)!=MOVE_OK)continue;
  if(target->Unlimbo(Cell_Coord(cell),DIR_N)){if(sam->In_Range(target->Target_Coord(),0)&&sam->Distance(target)>=18*ICON_LEPTON_W){placed=true;break;}target->Limbo();}
 }
 Check("move target into measured eighteen-to-twenty-cell range",placed);target->IsDiscoveredByPlayer=true;target->Assign_Mission(MISSION_SLEEP);target->Commence();
 int score=0;std::printf("RANGE_DIAG dist=%d range=%d coordRange=%d objectRange=%d discovered=%d active=%d limbo=%d legal=%d threat=%u target=%u\n",sam->Distance(target),sam->Weapon_Range(0),int(sam->In_Range(target->Target_Coord(),0)),int(sam->In_Range(target,0)),int(target->IsDiscoveredByPlayer),int(target->IsActive),int(target->IsInLimbo),int(sam->Evaluate_Object(THREAT_NORMAL,1<<RTTI_UNIT,0,target,score)),unsigned(sam->Greatest_Threat(THREAT_NORMAL)),unsigned(target->As_Target()));
 sam->Assign_Mission(MISSION_GUARD);sam->Commence();for(int i=0;i<1000&&!Bullets.Count();++i){++Frame;Logic.AI();}
 std::printf("RANGE_AFTER threat=%u tarcom=%u target=%u discovered=%d status=%d mission=%d distance=%d\n",unsigned(sam->Greatest_Threat(THREAT_NORMAL)),unsigned(sam->TarCom),unsigned(target->As_Target()),int(target->IsDiscoveredByPlayer),sam->Status,int(sam->Mission),sam->Distance(target));
 Check("actual SAM reacquires after target enters twenty cells",Bullets.Count()==1);
 Check("flight target remains near the new maximum range",sam->Distance(target)>=18*ICON_LEPTON_W);
 sam->Assign_Mission(MISSION_SLEEP);sam->Commence();int before=target->Strength;
 for(int i=0;i<1000&&Bullets.Count();++i){++Frame;Logic.AI();}
 Check("native half-speed missile physically reaches the near-twenty-cell target",target->Strength<before&&Bullets.Count()==0);
}
static void SV_Forced(const std::string& dir,bool miss,int population){
 auto sam=SV_Ready(dir);actors=SV_Enemies(sam,population);sam->Assign_Target(actors[0]);sam->TechnoClass::Fire_At(actors[0],0);int roots=Bullets.Count();
 Check("forced scenario has expected root count",roots==(population>=10?2:1));
 for(int generation=0;generation<6;++generation){ImpactGeneration(!miss);Check("each root continues singly for five ricochets then stops",Bullets.Count()==(generation<5?roots:0));
  if(generation==2||generation==4)Checkpoint(dir);
 }
 Check("all root and additional chains end together",Bullets.Count()==0);Check("misses cause no splash damage",!miss||Loss()==0);
}
static void SV_Legacy(const std::string& dir,int version,int used){
 auto sam=SV_Ready(dir);actors=SV_Enemies(sam,8);sam->Assign_Target(actors[0]);sam->TechnoClass::Fire_At(actors[0],0);
 Check("save current chain for legacy migration",Save_Game((dir+"/legacy.sav").c_str(),"Legacy in flight SAM"));
 {std::ifstream in(dir+"/legacy.sav",std::ios::binary);std::string b((std::istreambuf_iterator<char>(in)),{});auto pos=b.rfind("CNCCHN04");Check("new five-hop footer is version four",pos!=std::string::npos);if(pos==std::string::npos)return;
  std::uint32_t record[16]{};std::memcpy(record,b.data()+pos+12,sizeof(record));int oldLimit=version<=2?2:10;record[1]=oldLimit-used;record[4]=used;for(int i=0;i<used;++i)record[5+i]=actors[i%actors.size()];if(version==1)record[2]=25;
  b[pos+7]=char('0'+version);b.resize(pos+12+(version<=2?8:16)*4);std::memcpy(&b[pos+12],record,(version<=2?8:16)*4);std::ofstream out(dir+"/legacy.sav",std::ios::binary|std::ios::trunc);out.write(b.data(),b.size());}
 Check("load earlier chain without losing following game state",Load_Game((dir+"/legacy.sav").c_str()));int remaining=MAX(0,5-used);
 for(int n=0;n<=remaining;++n){ImpactGeneration(false);Check("legacy chain preserves consumed hops and uses new remaining budget",Bullets.Count()==(n<remaining?1:0));if(n==1)Checkpoint(dir);}
}
static unsigned SV_Observe(const std::string& dir,unsigned roots){
 CCFileClass out((dir+"/chain-observed.bin").c_str());out.Open(WRITE);Campaign_SAM_Chain_Save(out);out.Close();std::ifstream in(dir+"/chain-observed.bin",std::ios::binary);char header[8];in.read(header,8);std::uint32_t count;in.read(reinterpret_cast<char*>(&count),4);unsigned maximum=0;bool valid=count<=roots;
 for(unsigned i=0;i<count;++i){std::uint32_t record[16];in.read(reinterpret_cast<char*>(record),64);maximum=MAX(maximum,record[4]);valid=valid&&record[4]<=5&&record[1]+record[4]==5;}
 if(!valid)Check("native travelling chains never exceed five used hops or multiply",false);return maximum;
}
static void SV_Native(const std::string& dir,int population,int expected,bool capture){
 auto sam=SV_Ready(dir);actors=SV_Enemies(sam,population);unsigned elapsed=0;
 // Mission-linked enemies remain separate during the full game logic, just as scripted campaign actors do.
 auto mission=new TriggerClass;mission->Action=TriggerClass::ACTION_NONE;for(auto id:actors)As_Techno(id)->Trigger=mission;
 for(;elapsed<1000&&!Bullets.Count();++elapsed){++Frame;Logic.AI();}
 std::printf("NATIVE_AUTO population=%d expected=%d live=%d ticks=%u\n",population,expected,Bullets.Count(),elapsed);
 Check("native SAM guard acquires and fires the expected simultaneous volley",Bullets.Count()==expected&&elapsed<1000);
 sam->Assign_Mission(MISSION_SLEEP);sam->Commence();unsigned maximum=0,ticks=0;int initialLoss=Loss();
 for(;ticks<4000&&Bullets.Count();++ticks){++Frame;Logic.AI();maximum=MAX(maximum,SV_Observe(dir,expected));
  if(ticks==6){Check("save actual travelling volley",Save_Game((dir+"/native-flight.sav").c_str(),"Native SAM automatic volley"));Check("load actual travelling volley",Load_Game((dir+"/native-flight.sav").c_str()));}
  if(capture&&ticks==5){
   for(int c=0;c<MAP_CELL_TOTAL;++c)if(Map.In_Radar(c))Map.Map_Cell(c,PlayerPtr,true);
   auto s=static_cast<BuildingClass*>(As_Techno(sam->As_Target()));auto c=Coord_Cell(s->Coord);++ScenarioInit;Map.Set_Tactical_Position(Cell_Coord(XY_Cell(Cell_X(c)-12,Cell_Y(c)-9)));--ScenarioInit;Map.Activate(1);Map.Flag_To_Redraw(true);auto old=Set_Logic_Page(HidPage);Map.Draw_It(true);Set_Logic_Page(old);
   GraphicBufferClass image(HidPage.Get_Width(),HidPage.Get_Height(),nullptr,HidPage.Get_Width()*HidPage.Get_Height());HidPage.Blit(image);auto path=dir+"/native-sam-volley.pcx";Write_PCX_File(const_cast<char*>(path.c_str()),image,GamePalette);
   Check("save native visual scene",Save_Game((dir+"/savegame.090").c_str(),"SAM five missile live volley"));
  }
 }
 for(auto id:actors)if(auto object=As_Techno(id))object->Trigger=nullptr;
 std::printf("NATIVE_RESULT population=%d roots=%d ticks=%u max_used_hops=%u damage=%d live=%d\n",population,expected,ticks,maximum,Loss()-initialLoss,Bullets.Count());
 Check("real native flight reaches fifth ricochet and terminates",maximum==5&&Bullets.Count()==0&&ticks<4000);
}
bool Generate_Mission_Starts(const char* directory){std::string dir(directory);SV_Density(dir);SV_RangeAndScope(dir);SV_Retarget(dir);for(bool miss:{false,true})for(int n:{8,10})SV_Forced(dir,miss,n);for(int v:{1,2,3})for(int used=0;used<=(v<3?2:10);++used)SV_Legacy(dir,v,used);for(auto row:std::vector<std::pair<int,int>>{{9,1},{10,2},{20,3},{40,4},{80,5}})SV_Native(dir,row.first,row.second,row.first==80);std::printf("TOTAL FAILURES %d\n",failures);return failures==0;}
