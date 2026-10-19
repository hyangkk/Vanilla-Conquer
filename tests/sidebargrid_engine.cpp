#include "function.h"
#include "sidebarlayout.h"
#include <cstdio>
#include <string>
static int failures=0;
static void Check(const char* label,bool ok){std::printf("%s %s\n",ok?"PASS":"FAIL",label);std::fflush(stdout);if(!ok)++failures;}
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
 for(int slot=100;slot<=155;++slot){
  char name[64];std::snprintf(name,sizeof(name),"/starts/savegame.%03d",slot);
  if(!Load_Game((dir+name).c_str()))return false;
  Check("old save loads with rebuilt eight-slot grid",Map.MaxVisible==8&&Map.SideBarWidth==296&&Map.SideX==344&&Map.RadWidth==296);
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
 Check("both categories contain more than eight entries",Map.Column[0].BuildableCount>8&&Map.Column[1].BuildableCount>8);
 for(int column=0;column<2;++column){
  auto& strip=Map.Column[column];
  Check("category icons stay inside screen",strip.X+strip.LeftEdgeOffset+strip.ObjectWidth*2+2<=SeenBuff.Get_Width());
  for(int slot=0;slot<8;++slot){
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
 Check("tactical viewport excludes entire sidebar",Map.TacLeptonWidth/ICON_LEPTON_W<=(344/ICON_PIXEL_W));
 Check("clip window is exactly four rows high",WindowList[WINDOW_SIDEBAR][WINDOWHEIGHT]==4*48);
 std::string output=dir+"/savegame.090";
 Check("save grid preview",Save_Game(output.c_str(),"SIDEBAR GRID PREVIEW"));
 int buildings=Buildings.Count();
 Check("save reload preserves content and layout",Load_Game(output.c_str())&&Buildings.Count()==buildings&&Map.Column[0].BuildableCount>8&&Map.Column[1].BuildableCount>8&&Map.SideX==344);
 std::printf("COUNTS buildings=%d units=%d\n",Map.Column[0].BuildableCount,Map.Column[1].BuildableCount);
 std::printf("TOTAL FAILURES %d\n",failures);return failures==0;
}
