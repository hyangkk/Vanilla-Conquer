#include "function.h"
#include "campaigntech.h"
#include "common/settings.h"
#include <cstdio>
#include <string>
bool Generate_Mission_Starts(const char* directory) {
 int failures=0;
 for(int slot=151;slot<=155;++slot){
  char filename[256];std::snprintf(filename,sizeof(filename),"%s/savegame.%03d",directory,slot);
  ScenPlayer=SCEN_PLAYER_GDI;Special.Init();
  if(!Load_Game(filename)) return false;
  GameActive=true;
  int ownBuildings=0,ownMCV=0,ownDinos=0;
  for(int i=0;i<Buildings.Count();++i)if(Buildings.Ptr(i)->House==PlayerPtr)++ownBuildings;
  for(int i=0;i<Units.Count();++i)if(Units.Ptr(i)->House==PlayerPtr){if(Units.Ptr(i)->Class->Type==UNIT_MCV)++ownMCV;if(Units.Ptr(i)->Class->Type>=UNIT_TRIC)++ownDinos;}
  auto& obelisk=BuildingTypeClass::As_Reference(STRUCT_OBELISK);
  auto& tower=BuildingTypeClass::As_Reference(STRUCT_ATOWER);
  std::printf("%s slot=%d house=%d actlike=%d human=%d jurassic=%d assets=%d option=%d unlocked=%d buildings=%d mcv=%d dinosaur-units=%d obelisk=%d advanced-tower=%d\n",Scen.ScenarioName,slot,int(PlayerPtr->Class->House),int(PlayerPtr->ActLike),int(PlayerPtr->IsHuman),int(Special.IsJurassic),int(AreThingiesEnabled),int(Settings.Options.CampaignAllTech),int(Campaign_All_Tech(PlayerPtr)),ownBuildings,ownMCV,ownDinos,int(PlayerPtr->Can_Build(&obelisk,PlayerPtr->ActLike)),int(PlayerPtr->Can_Build(&tower,PlayerPtr->ActLike)));
  std::fflush(stdout);
  if(!Special.IsJurassic || ScenPlayer!=SCEN_PLAYER_JP)++failures;
  if(!Campaign_All_Tech(PlayerPtr)||!PlayerPtr->Can_Build(&obelisk,PlayerPtr->ActLike)||!PlayerPtr->Can_Build(&tower,PlayerPtr->ActLike))++failures;
  if(slot==154) {
   UnitClass* mcv=nullptr;
   for(int i=0;i<Units.Count();++i)if(Units.Ptr(i)->House==PlayerPtr&&Units.Ptr(i)->Class->Type==UNIT_MCV)mcv=Units.Ptr(i);
   bool deployed=false;
   if(mcv) {
    mcv->Limbo();
    for(int y=Map.MapCellY+2;y<Map.MapCellY+Map.MapCellHeight-3&&!deployed;++y)
     for(int x=Map.MapCellX+2;x<Map.MapCellX+Map.MapCellWidth-3&&!deployed;++x) {
      if(!BuildingTypeClass::As_Reference(STRUCT_CONST).Legal_Placement(XY_Cell(x-1,y-1)))continue;
      if(!mcv->Unlimbo(Cell_Coord(XY_Cell(x,y)),DIR_SW))continue;
      mcv->Assign_Destination(TARGET_NONE);mcv->IsRotating=false;mcv->PrimaryFacing=DIR_SW;
      deployed=mcv->Try_To_Deploy();if(!deployed)mcv->Limbo();
     }
   }
   bool factories=deployed&&obelisk.Who_Can_Build_Me(true,true,PlayerPtr->Class->House)&&tower.Who_Can_Build_Me(true,true,PlayerPtr->Class->House);
   std::printf("BONUS4 MCV deployment=%d both-faction-construction-factory=%d\n",int(deployed),int(factories));std::fflush(stdout);if(!factories)++failures;
  }
  bool previous=Settings.Options.CampaignAllTech;Settings.Options.CampaignAllTech=false;
  if(Campaign_All_Tech(PlayerPtr))++failures;Settings.Options.CampaignAllTech=previous;
  HouseClass* enemy=HouseClass::As_Pointer(HOUSE_BAD);if(enemy&&enemy!=PlayerPtr&&Campaign_All_Tech(enemy))++failures;
 }
 for(int slot : {100,150}) {
  char filename[256];std::snprintf(filename,sizeof(filename),"%s/control.%03d",directory,slot);
  if(!Load_Game(filename) || Special.IsJurassic || !Campaign_All_Tech(PlayerPtr))++failures;
 }
 std::printf("TOTAL FAILURES %d\n",failures);return failures==0;
}
