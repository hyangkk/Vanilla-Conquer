// Offline verification-only replacement for missionstarts.cpp; never install this build.
#include "function.h"
#include "campaigntech.h"
#include <cstdio>
#include <fstream>
#include <string>
static int failures;
static void Check(const char* label,bool ok){std::printf("%s %s\n",ok?"PASS":"FAIL",label);std::fflush(stdout);if(!ok)++failures;}
bool Generate_Mission_Starts(const char* directory){
 std::string dir(directory);std::ifstream input(dir+"/missions.txt");std::string root;int slot=200;
 while(std::getline(input,root)){
  Special.Init();AreThingiesEnabled=false;GameToPlay=GAME_NORMAL;PlayerWins=PlayerLoses=false;
  char name[32];std::snprintf(name,sizeof(name),"/savegame.%03d",slot++);
  Check("expansion save cold-loads",Load_Game((dir+name).c_str()));
  if(!PlayerPtr)return false;
  bool music=false;for(int song=THEME_FIRST;song<=THEME_LAST;++song)if(Theme.Is_Allowed(ThemeType(song)))music=true;
  Check("disc search path has a playable soundtrack",music);
  Check("mission identity and frame zero",root==Scen.ScenarioName&&Frame==0);
  CCFileClass file((root+".INI").c_str());CCINIClass ini;
  Check("opening scenario INI loads",ini.Load(file,false));
  Check("opening credits match original scenario",PlayerPtr->Credits==ini.Get_Int(PlayerPtr->Class->IniName,"Credits",0)*100);
  Check("all tech and power survive cold load",Campaign_All_Tech(PlayerPtr)&&PlayerPtr->Power>=1000000);
  if(root=="SCG72EA")Check("PATSUX retains laser Orcas",AircraftTypeClass::As_Reference(AIRCRAFT_ORCA).Primary==WEAPON_OBELISK_LASER);
  else Check("ordinary missions retain standard Orcas",AircraftTypeClass::As_Reference(AIRCRAFT_ORCA).Primary==WEAPON_DRAGON);
  if(root=="SCG71EA"){
   Check("N64 snow map uses native winter theater",Map.Theater==THEATER_WINTER);
   int missing=0;
   for(int y=Map.MapCellY;y<Map.MapCellY+Map.MapCellHeight;++y)
    for(int x=Map.MapCellX;x<Map.MapCellX+Map.MapCellWidth;++x){
     char templateName[64];int icon;void* image=nullptr;
     if(!Map[XY_Cell(x,y)].Get_Template_Info(templateName,icon,image)||!image)++missing;
    }
   std::printf("N64 missing template cells: %d\n",missing);
   Check("N64 terrain graphics exist for every map cell",missing==0);
  }
 }
 Check("all twenty-five expansion starts checked",slot==225);
 std::printf("TOTAL FAILURES %d\n",failures);return failures==0;
}
