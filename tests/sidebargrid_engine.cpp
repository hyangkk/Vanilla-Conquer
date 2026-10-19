#include "function.h"
#include "sidebarlayout.h"
#include <cstdio>
#include <string>
#include <type_traits>
#include <vector>
using SelectionBase = std::remove_reference<decltype(SidebarClass::StripClass::SelectButton[0][0])>::type;
struct TestSelection : SelectionBase { using SelectionBase::Action; };
static int failures=0;
static void Check(const char* label,bool ok){std::printf("%s %s\n",ok?"PASS":"FAIL",label);std::fflush(stdout);if(!ok)++failures;}
static void ClearSight() {
 for(int i=0;i<MAP_CELL_TOTAL;++i){Map[i].IsMapped=false;Map[i].IsVisible=false;Map[i].IsMappedByPlayerMask=0;Map[i].IsVisibleByPlayerMask=0;}
}
static std::vector<bool> Sight(HouseClass* house) {
 std::vector<bool> result;for(int i=0;i<MAP_CELL_TOTAL;++i)result.push_back(Map[i].Is_Mapped(house));return result;
}
static BuildingClass* Place(StructType type){
 BuildingClass* b=new BuildingClass(type,PlayerPtr->Class->House);
 for(int y=Map.MapCellY+2;y<Map.MapCellY+Map.MapCellHeight-4;++y)
  for(int x=Map.MapCellX+2;x<Map.MapCellX+Map.MapCellWidth-4;++x)
   if(b->Can_Enter_Cell(XY_Cell(x,y))==MOVE_OK&&b->Unlimbo(Cell_Coord(XY_Cell(x,y)),DIR_N)){
    b->Assign_Mission(MISSION_GUARD);b->Commence();b->Grand_Opening();b->Revealed(PlayerPtr);b->Update_Buildables();return b;
   }
 return nullptr;
}
bool Generate_Mission_Starts(const char* directory){
 std::string dir(directory);
 GameToPlay=GAME_NORMAL; GameActive=true; PlayerWins=PlayerLoses=false;
 Check("expanded logical canvas is active",SeenBuff.Get_Width()==960&&SeenBuff.Get_Height()==600);
 for(int slot=100;slot<=155;++slot){
  char name[64];std::snprintf(name,sizeof(name),"/starts/savegame.%03d",slot);
  if(!Load_Game((dir+name).c_str()))return false;
  Check("old save loads with rebuilt height-adaptive grid",Map.MaxVisible==SidebarLayout::VisibleCount()&&Map.SideBarWidth==240&&Map.SideX==SeenBuff.Get_Width()-240&&Map.RadWidth==240);
  Check("legacy save expands viewport with sidebar shown or hidden",Lepton_To_Pixel(Map.TacLeptonWidth)==SeenBuff.Get_Width()-(Map.IsSidebarActive?240:0)&&Lepton_To_Pixel(Map.TacLeptonHeight)==SeenBuff.Get_Height()-Map.Get_Tab_Height());
 }
 if(!Load_Game((dir+"/progress.sav").c_str()))return false;
 GameActive=true;PlayerWins=PlayerLoses=false;PlayerPtr->Credits=50000;
 PlayerPtr->IsToWin=PlayerPtr->IsToLose=PlayerPtr->IsToDie=false;
 Options.Set();
 for(StructType type : {STRUCT_BARRACKS,STRUCT_WEAP,STRUCT_HELIPAD,STRUCT_RADAR})Check("place preview production facility",Place(type)!=nullptr);
 PlayerPtr->IsRecalcNeeded=true;PlayerPtr->AI();
 PlayerPtr->IsToWin=PlayerPtr->IsToLose=PlayerPtr->IsToDie=false;
 PlayerWins=PlayerLoses=false;
 Map.Activate(1);Map.Init_IO();
 Check("both categories contain more than sixteen entries",Map.Column[0].BuildableCount>16&&Map.Column[1].BuildableCount>16);
 for(int column=0;column<2;++column){
  auto& strip=Map.Column[column];
  Check("category icons stay inside screen",strip.X+strip.LeftEdgeOffset+strip.ObjectWidth*2+2<=SeenBuff.Get_Width());
  for(int slot=0;slot<Map.MaxVisible;++slot){
   auto& button=SidebarClass::StripClass::SelectButton[column][slot];
   Check("click rectangle matches rendered grid cell",button.X==strip.X+strip.LeftEdgeOffset+(slot%2)*(strip.ObjectWidth+2)&&button.Y==strip.Y+(slot/2)*strip.ObjectHeight-1&&button.Width==strip.ObjectWidth&&button.Height==strip.ObjectHeight);
  }
  strip.TopIndex=0;int previous=0;
  while(strip.Scroll(false)){Check("scroll advances exactly one complete row",strip.TopIndex==previous+2);previous=strip.TopIndex;}
  Check("scroll reaches final entries without overflow",strip.TopIndex==SidebarLayout::MaximumTop(strip.BuildableCount)&&strip.TopIndex<strip.BuildableCount);
  while(strip.Scroll(true)){}
  Check("scroll returns to first row",strip.TopIndex==0&&!strip.Scroll(true));
  strip.TopIndex=17;strip.Slid=7;strip.IsScrolling=true;strip.Init_IO(column);
  Check("legacy scroll state normalized on load",strip.TopIndex%2==0&&strip.TopIndex<=SidebarLayout::MaximumTop(strip.BuildableCount)&&!strip.IsScrolling&&strip.Slid==0);
  strip.TopIndex=0;
 }
 Check("tactical viewport excludes entire sidebar",Map.TacLeptonWidth/ICON_LEPTON_W<=((SeenBuff.Get_Width()-240)/ICON_PIXEL_W));
 Check("loaded save expands tactical height",Lepton_To_Pixel(Map.TacLeptonHeight)==600-Map.Get_Tab_Height());
 Check("clip window is exactly all visible enlarged rows high",WindowList[WINDOW_SIDEBAR][WINDOWHEIGHT]==SidebarLayout::ActiveRows()*36);
 std::string output=dir+"/savegame.090";
 for(StructType type : {STRUCT_OBELISK,STRUCT_ATOWER}) {
  BuildingClass* tower=Place(type);Check("place sight test tower",tower!=nullptr);if(!tower)return false;
  int base=tower->Class->SightRange;
  ClearSight();Map.Sight_From(PlayerPtr,Coord_Cell(tower->Center_Coord()),base*2,false);auto expected=Sight(PlayerPtr);
  ClearSight();tower->Look();Check("own tower reveals exact doubled radius",Sight(PlayerPtr)==expected);
  ClearSight();Map.Sight_From(PlayerPtr,Coord_Cell(tower->Center_Coord()),base,false);Check("double radius reveals additional fog",Sight(PlayerPtr)!=expected);
  HouseClass* enemy=HouseClass::As_Pointer(PlayerPtr->Class->House==HOUSE_GOOD?HOUSE_BAD:HOUSE_GOOD);
  tower->House=enemy;ClearSight();Map.Sight_From(enemy,Coord_Cell(tower->Center_Coord()),base,false);auto ordinary=Sight(enemy);
  ClearSight();tower->Look();Check("enemy tower keeps original sight",Sight(enemy)==ordinary);
  tower->House=PlayerPtr;
  ClearSight();std::string sightSave=dir+"/tower-sight.sav";Check("save existing tower with unrevealed fog",Save_Game(sightSave.c_str(),"SIGHT CHECK"));
  Check("old saved tower gains doubled sight on load",Load_Game(sightSave.c_str()));
  auto loaded=Sight(PlayerPtr);bool contains=true;for(int i=0;i<MAP_CELL_TOTAL;++i)if(expected[i]&&!loaded[i])contains=false;
  Check("load fixup reveals full doubled tower radius",contains);
 }
 Check("sidebar width is exactly fifty percent larger",Map.SideBarWidth==240);
 Check("full-height grid and scroll buttons fit",Map.Column[0].Y+SidebarLayout::ActiveRows()*36<=SidebarClass::StripClass::UpButton[0].Y&&SidebarClass::StripClass::UpButton[0].Y+SidebarClass::StripClass::UpButton[0].Height==SeenBuff.Get_Height());
 Check("icons enlarged by fifty percent",Map.Column[0].ObjectWidth==48&&Map.Column[0].ObjectHeight==36);
 Check("power gauge uses full height",Map.PowY+Map.PowHeight==600);
 Check("save grid preview",Save_Game(output.c_str(),"SIDEBAR GRID PREVIEW"));
 int buildings=Buildings.Count();
 Check("save reload preserves content and layout",Load_Game(output.c_str())&&Buildings.Count()==buildings&&Map.Column[0].BuildableCount>16&&Map.Column[1].BuildableCount>16&&Map.SideX==SeenBuff.Get_Width()-240);
 // Exercise the real selection action for every visible cell, not only its rectangle.
 for(int category=0;category<2;++category){
  auto& strip=Map.Column[category];strip.TopIndex=0;
  for(int slot=0;slot<Map.MaxVisible;++slot){
   OutList.Init();KeyNumType key=KN_LMOUSE;
   (SidebarClass::StripClass::SelectButton[category][slot].*&TestSelection::Action)(GadgetClass::LEFTPRESS,key);
   Check("grid cell queues matching production",OutList.Count==1&&OutList[0].Type==EventClass::PRODUCE&&OutList[0].Data.Specific.Type==strip.Buildables[slot].BuildableType&&OutList[0].Data.Specific.ID==strip.Buildables[slot].BuildableID);
  }
 }
 OutList.Init();
 std::printf("COUNTS buildings=%d units=%d\n",Map.Column[0].BuildableCount,Map.Column[1].BuildableCount);
 std::printf("TOTAL FAILURES %d\n",failures);return failures==0;
}
