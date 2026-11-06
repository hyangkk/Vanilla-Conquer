// Real native logic regression. Use a private campaign/save directory only.
#include "../tests/samtest_support.h"
#include "campaignrepair.h"
#include "common/filepcx.h"
static void ZeroMoney(const std::string& dir){
 Check("load latest real user save copy",Load_Game((dir+"/user-current.sav").c_str()));Options.Set();GameToPlay=GAME_NORMAL;GameActive=true;
 std::printf("LIVE_SAVE frame=%d money=%d infantry=%d units=%d aircraft=%d\n",Frame,PlayerPtr->Available_Money(),Infantry.Count(),Units.Count(),Aircraft.Count());
 Reset(dir);PlayerPtr->Credits=0;PlayerPtr->Tiberium=0;
 auto infantry=new InfantryClass(INFANTRY_E1,HOUSE_GOOD);Check("place damaged native rifleman",Put(infantry,Center()));infantry->Assign_Mission(MISSION_SLEEP);infantry->Commence();infantry->Strength=10;auto id=infantry->As_Target();
 for(int tick=0;tick<45;++tick){++Frame;Logic.AI();}
 auto after=As_Techno(id);std::printf("ZERO_FUNDS_INFANTRY before=10 after=%d money=%d\n",after?after->Strength:-1,PlayerPtr->Available_Money());
 Check("infantry regenerates in real logic even without money",after&&after->Strength>10);
}

static void PrepareGround(){
 for(int i=Terrains.Count()-1;i>=0;--i)Terrains.Ptr(i)->Limbo();
 for(int c=0;c<MAP_CELL_TOTAL;++c)if(Map.In_Radar(c)){Map[c].TType=TEMPLATE_CLEAR1;Map[c].TIcon=0;Map[c].Overlay=OVERLAY_NONE;Map[c].Recalc_Attributes();Map.Map_Cell(c,PlayerPtr,true);}
 PlayerPtr->IsToWin=PlayerPtr->IsToLose=PlayerPtr->IsToDie=false;
}
static void ActualTypes(const std::string& dir){
 Reset(dir);PrepareGround();PlayerPtr->Credits=0;PlayerPtr->Tiberium=0;
 std::vector<TARGET> ids;std::vector<int> before;
 std::vector<TechnoClass*> objects;
 for(int i=0;i<INFANTRY_COUNT;++i)objects.push_back(new InfantryClass(InfantryType(i),HOUSE_GOOD));
 for(auto type:{UNIT_MTANK,UNIT_BIKE,UNIT_MCV,UNIT_HARVESTER,UNIT_SIEGE_TANK,UNIT_SIEGE_MODE})objects.push_back(new UnitClass(type,HOUSE_GOOD));
 for(auto type:{AIRCRAFT_ORCA,AIRCRAFT_TRANSPORT}){auto a=new AircraftClass(type,HOUSE_GOOD);a->Altitude=AircraftClass::FLIGHT_LEVEL;objects.push_back(a);}
 for(unsigned i=0;i<objects.size();++i){auto o=objects[i];CELL c=XY_Cell(Map.MapCellX+4+(i%7)*3,Map.MapCellY+4+(i/7)*3);Check("place actual player object for full logic regeneration",Put(o,c));o->Assign_Mission(MISSION_SLEEP);o->Commence();o->Strength=MAX(1,Campaign_Max_Health(o)/2);ids.push_back(o->As_Target());before.push_back(o->Strength);}
 for(int tick=0;tick<45;++tick){++Frame;Logic.AI();}
 for(unsigned i=0;i<ids.size();++i){auto o=As_Techno(ids[i]);std::printf("NATIVE_HEAL index=%u kind=%d before=%d after=%d max=%d\n",i,o?int(o->What_Am_I()):-1,before[i],o?int(o->Strength):-1,o?Campaign_Max_Health(o):-1);Check("actual full Logic.AI heals each placed type without funds",o&&o->IsActive&&o->Strength>before[i]&&o->Strength<=Campaign_Max_Health(o));}
 Check("mobile regeneration never spends or grants money",PlayerPtr->Available_Money()==0);
 Check("save regenerated army",Save_Game((dir+"/healed.sav").c_str(),"ALL UNIT REGENERATION"));
 Check("reload regenerated army",Load_Game((dir+"/healed.sav").c_str()));
 for(unsigned i=0;i<ids.size();++i){auto o=As_Techno(ids[i]);Check("healed health persists through save/reload",o&&o->Strength>before[i]);}
 for(int c=0;c<MAP_CELL_TOTAL;++c)if(Map.In_Radar(c))Map.Map_Cell(c,PlayerPtr,true);
 ++ScenarioInit;Map.Set_Tactical_Position(Cell_Coord(XY_Cell(Map.MapCellX+2,Map.MapCellY+2)));--ScenarioInit;
 Special.HealthBarDisplayMode=SpecialClass::HB_ALWAYS;
 auto selected=As_Techno(ids.front());if(selected)selected->Select();
 auto old=Set_Logic_Page(HidPage);Map.Draw_It(true);Set_Logic_Page(old);
 GraphicBufferClass image(HidPage.Get_Width(),HidPage.Get_Height(),nullptr,HidPage.Get_Width()*HidPage.Get_Height());HidPage.Blit(image);std::string path=dir+"/healed-army.pcx";Write_PCX_File(const_cast<char*>(path.c_str()),image,GamePalette);
}
static void Filters(const std::string& dir){
 Reset(dir);Frame=120;PlayerPtr->Credits=0;PlayerPtr->Tiberium=0;
 auto o=new InfantryClass(INFANTRY_E1,HOUSE_BAD);Check("place enemy control",Put(o,Center()));o->Strength=10;Campaign_Auto_Repair(o);Check("enemy healing unchanged",o->Strength==10);
 o->House=PlayerPtr;o->Strength=0;Campaign_Auto_Repair(o);Check("dead objects never resurrect",o->Strength==0);
 o->Strength=10;GameToPlay=GAME_SKIRMISH;Campaign_Auto_Repair(o);Check("multiplayer/skirmish unchanged",o->Strength==10);GameToPlay=GAME_NORMAL;
 o->Strength=Campaign_Max_Health(o)-1;Campaign_Auto_Repair(o);Check("healing clamps to max HP",o->Strength==Campaign_Max_Health(o));Campaign_Auto_Repair(o);Check("full HP stays at max",o->Strength==Campaign_Max_Health(o));
}
bool Generate_Mission_Starts(const char* directory){ZeroMoney(directory);ActualTypes(directory);Filters(directory);std::printf("TOTAL FAILURES %d\n",failures);return failures==0;}
