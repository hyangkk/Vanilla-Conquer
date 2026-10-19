// Offline native probe; temporarily substitutes missionstarts.cpp, never shipped.
#include "function.h"
#include <cstdio>
#include <string>
#include <vector>
static int failures;
static void Check(const char* label,bool ok){std::printf("%s %s\n",ok?"PASS":"FAIL",label);std::fflush(stdout);if(!ok)++failures;}
static void Clear(){for(int i=0;i<MAP_CELL_TOTAL;++i){Map[i].IsMapped=false;Map[i].IsVisible=false;Map[i].IsMappedByPlayerMask=0;Map[i].IsVisibleByPlayerMask=0;}}
static std::vector<bool> Sight(HouseClass* h){std::vector<bool> v;for(int i=0;i<MAP_CELL_TOTAL;++i)v.push_back(Map[i].Is_Mapped(h));return v;}
static std::vector<bool> Expected(HouseClass* h,CELL c,int radius,bool incremental=false){
 Clear();for(int i=0;i<MAP_CELL_TOTAL;++i){int d=Distance(CELL(i),c);if(d<=radius&&(!incremental||d>radius-2))Map.Map_Cell(i,h,true);}return Sight(h);
}
bool Generate_Mission_Starts(const char* directory){
 std::string dir(directory);if(!Load_Game((dir+"/fixture.sav").c_str()))return false;
 for(int i=0;i<Triggers.Count();++i)Triggers.Ptr(i)->Action=TriggerClass::ACTION_NONE;
 Options.Set();GameToPlay=GAME_NORMAL;GameActive=true;PlayerWins=PlayerLoses=false;
 auto good=HouseClass::As_Pointer(HOUSE_GOOD);auto bad=HouseClass::As_Pointer(HOUSE_BAD);
 PlayerPtr=good;good->IsHuman=true;bad->IsHuman=false;good->Power=1000000;
 for(int i=Units.Count()-1;i>=0;--i)Units.Ptr(i)->Limbo();
 for(int i=Infantry.Count()-1;i>=0;--i)Infantry.Ptr(i)->Limbo();
 for(int i=Buildings.Count()-1;i>=0;--i)Buildings.Ptr(i)->Limbo();
 for(int i=Aircraft.Count()-1;i>=0;--i)Aircraft.Ptr(i)->Limbo();
 auto sam=new BuildingClass(STRUCT_SAM,HOUSE_GOOD);bool placed=false;int px=0,py=0;
 for(int y=Map.MapCellY+18;y<Map.MapCellY+Map.MapCellHeight-18&&!placed;++y)
  for(int x=Map.MapCellX+18;x<Map.MapCellX+Map.MapCellWidth-18;++x)
   if(sam->Can_Enter_Cell(XY_Cell(x,y))==MOVE_OK&&sam->Unlimbo(Cell_Coord(XY_Cell(x,y)),DIR_N)){px=x;py=y;placed=true;break;}
 Check("place SAM away from edges",placed);if(!placed)return false;
 CELL center=Coord_Cell(sam->Center_Coord());Check("base SAM sight stays three",sam->Class->SightRange==3);
 auto expected=Expected(good,center,15);Clear();sam->Look();Check("player SAM reveals exact fifteen cell radius",Sight(good)==expected);
 Check("fourteen cells away is revealed",Map[XY_Cell(Cell_X(center)+14,Cell_Y(center))].Is_Mapped(good));
 Check("eighteen cells away stays shrouded",!Map[XY_Cell(Cell_X(center)+18,Cell_Y(center))].Is_Mapped(good));
 sam->House=bad;auto ordinary=Expected(bad,center,3);Clear();sam->Look();Check("enemy SAM retains three cell sight",Sight(bad)==ordinary);sam->House=good;
 GameToPlay=GAME_SKIRMISH;ordinary=Expected(good,center,3);Clear();sam->Look();Check("skirmish sight unchanged",Sight(good)==ordinary);GameToPlay=GAME_NORMAL;
 good->IsHuman=false;Clear();sam->Look();Check("AI player does not receive sight bonus",Sight(good)==ordinary);good->IsHuman=true;
 sam->House=bad;PlayerPtr=bad;bad->IsHuman=true;expected=Expected(bad,center,15);Clear();sam->Look();Check("Nod player gets fifteen cells",Sight(bad)==expected);sam->House=good;PlayerPtr=good;bad->IsHuman=false;
 // Legacy radius ten uses its original hand-authored offset table, not an exact disk.
 for(int radius : {3,8,15})for(CELL cell : {center,XY_Cell(Map.MapCellX,Map.MapCellY),XY_Cell(Map.MapCellX+Map.MapCellWidth-1,Map.MapCellY+Map.MapCellHeight-1)}){
  std::printf("CASE radius=%d cell=%d\n",radius,int(cell));auto reference=Expected(good,cell,radius);Clear();Map.Sight_From(good,cell,radius,false);Check("full radius and map edge reveal match exhaustive reference",Sight(good)==reference);
 }
 expected=Expected(good,center,15,true);Clear();Map.Sight_From(good,center,15,true);Check("large incremental sight reveals outer ring",Sight(good)==expected);
 expected=Expected(good,center,15);Clear();Check("save unrevealed existing SAM",Save_Game((dir+"/old-sam.sav").c_str(),"SAM BEFORE SIGHT"));
 Check("load existing SAM save",Load_Game((dir+"/old-sam.sav").c_str()));auto loaded=Sight(PlayerPtr);bool contains=true;for(int i=0;i<MAP_CELL_TOTAL;++i)if(expected[i]&&!loaded[i])contains=false;Check("load fixup reveals all fifteen cells",contains);
 Clear();for(int i=0;i<Buildings.Count();++i)if(!Buildings.Ptr(i)->IsInLimbo)Buildings.Ptr(i)->Look();
 ++ScenarioInit;Map.Set_Tactical_Position(Cell_Coord(XY_Cell(px-15,py-10)));--ScenarioInit;
 PlayerPtr->Credits=50000;PlayerPtr->IsToWin=PlayerPtr->IsToLose=PlayerPtr->IsToDie=false;
 Check("save sight preview",Save_Game((dir+"/savegame.096").c_str(),"SAM SIGHT 15 CELLS"));
 std::printf("TOTAL FAILURES %d\n",failures);return failures==0;
}
