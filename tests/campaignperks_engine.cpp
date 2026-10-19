#include "function.h"
#include <cstdio>
#include <string>
static int failures=0;
static void Check(const char* label,bool ok){std::printf("%s %s\n",ok?"PASS":"FAIL",label);std::fflush(stdout);if(!ok)++failures;}
bool Generate_Mission_Starts(const char* directory){
 const std::string dir(directory);
 for(int slot=100;slot<=155;++slot){
  char name[64];std::snprintf(name,sizeof(name),"/starts/savegame.%03d",slot);
  if(!Load_Game((dir+name).c_str()))return false;
  Check("existing mission start has unlimited power",PlayerPtr->Power>=1000000&&PlayerPtr->Power_Fraction()>=1);
 }
 if(!Load_Game((dir+"/progress.sav").c_str()))return false;
 Check("in-progress save gains unlimited power",Frame>0&&PlayerPtr->Power>=1000000&&PlayerPtr->Power_Fraction()>=1);
 HouseClass* human=PlayerPtr;HouseClass* enemy=HouseClass::As_Pointer(HOUSE_BAD);
 int maxDrain=0;
 for(int i=0;i<STRUCT_COUNT;++i)maxDrain=MAX(maxDrain,BuildingTypeClass::As_Reference(StructType(i)).Drain);
 Check("power reserve exceeds maximum possible map drain",human->Power>maxDrain*BUILDING_MAX);
 human->Power=0;human->Drain=1000;GameActive=true;human->AI();
 Check("power replenishes on normal simulation tick",human->Power>=1000000&&human->Power_Fraction()>=1);
 enemy->Power=0;enemy->Drain=1000;enemy->AI();
 Check("enemy retains low power",enemy->Power==0&&enemy->Power_Fraction()<1);
 auto& type=BuildingTypeClass::As_Reference(STRUCT_POWER);
 CELL far=-1; GameToPlay=GAME_SKIRMISH;
 for(int y=Map.MapCellY+2;y<Map.MapCellY+Map.MapCellHeight-3&&far<0;++y)
  for(int x=Map.MapCellX+2;x<Map.MapCellX+Map.MapCellWidth-3;++x){
   CELL c=XY_Cell(x,y);
   if(type.Legal_Placement(c)&&!Map.Passes_Proximity_Check(&type,human->Class->House,type.Occupy_List(true),c)
      &&!Map.Passes_Proximity_Check(&type,enemy->Class->House,type.Occupy_List(true),c)){far=c;break;}
  }
 Check("find legal ground outside base proximity",far>=0);if(far<0)return false;
 BuildingClass* pending=new BuildingClass(STRUCT_POWER,human->Class->House);
 Check("skirmish UI keeps proximity limit",!Map.Passes_Proximity_Check(&type,human->Class->House,type.Occupy_List(true),far));
 Check("skirmish placement keeps proximity limit",!pending->Passes_Proximity_Check(far));
 GameToPlay=GAME_NORMAL;
 Check("player UI allows distant legal placement",Map.Passes_Proximity_Check(&type,human->Class->House,type.Occupy_List(true),far));
 Check("player placement allows distant legal placement",pending->Passes_Proximity_Check(far));
 Check("enemy UI retains proximity limit",!Map.Passes_Proximity_Check(&type,enemy->Class->House,type.Occupy_List(true),far));
 FactoryClass* factory=new FactoryClass;factory->Set(*pending);human->Set_Factory(RTTI_BUILDINGTYPE,factory);
 Check("completed player building actually placed remotely",human->Place_Object(RTTI_BUILDINGTYPE,far));
 Check("placed structure occupies chosen cell",Map[far].Cell_Building()==pending);
 Check("overlap rejected in player UI",!Map.Passes_Proximity_Check(&type,human->Class->House,type.Occupy_List(true),far));
 Check("out-of-map rejected in player UI",!Map.Passes_Proximity_Check(&type,human->Class->House,type.Occupy_List(true),-1));
 BuildingClass* overlap=new BuildingClass(STRUCT_POWER,human->Class->House);
 Check("overlap rejected in actual placement",!overlap->Passes_Proximity_Check(far));
 const std::string saved=dir+"/roundtrip.sav";int money=human->Available_Money();
 Check("save constructed state",Save_Game(saved.c_str(),"remote construction check"));
 Check("reload preserves building power and money",Load_Game(saved.c_str())&&Map[far].Cell_Building()!=nullptr&&PlayerPtr->Power>=1000000&&PlayerPtr->Available_Money()==money);
 GameToPlay=GAME_SKIRMISH;PlayerPtr->Power=0;PlayerPtr->Drain=1000;PlayerPtr->AI();
 Check("skirmish power remains unchanged",PlayerPtr->Power==0);
 GameToPlay=GAME_NORMAL;
 char mission[]="SCG01EA";ScenPlayer=SCEN_PLAYER_GDI;Scen.Scenario=1;ScenDir=SCEN_DIR_EAST;ScenVar=SCEN_VAR_A;
 Check("fresh mission starts with unlimited power",Read_Scenario(mission)&&PlayerPtr->Power>=1000000&&PlayerPtr->Power_Fraction()>=1);
 std::printf("TOTAL FAILURES %d\n",failures);return failures==0;
}
