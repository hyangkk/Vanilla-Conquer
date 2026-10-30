// Native private regression; substituted for missionstarts.cpp only by the probe harness.
#define Generate_Mission_Starts SAM_Regression
#include "../tests/samblast_engine.cpp"
#undef Generate_Mission_Starts
#include "siegetank.h"
static UnitClass* Tank(const std::string& dir) {
    Reset(dir); auto tank=new UnitClass(UNIT_SIEGE_TANK,HOUSE_GOOD);
    Check("place the new production unit",Put(tank,Center()));
    tank->Assign_Mission(MISSION_SLEEP);tank->Commence();
    HouseClass::As_Pointer(HOUSE_BAD)->ArmorBias=1;
    PlayerPtr->ArmorBias=1;PlayerPtr->Credits=100000;
    return tank;
}
static UnitClass* Enemy(UnitClass* tank,int tiles) {
    auto target=new UnitClass(UNIT_MTANK,HOUSE_BAD);
    Check("place enemy target",Put(target,XY_Cell(Cell_X(Coord_Cell(tank->Coord))+tiles,Cell_Y(Coord_Cell(tank->Coord)))));
    target->Strength=10000;target->IsDiscoveredByPlayer=true;
    target->Assign_Mission(MISSION_SLEEP);target->Commence();
    Campaign_Enemies_Mark_Extra(target);return target;
}
static void Deploy(UnitClass* tank) {
    Check("begin siege conversion",SiegeTank::Toggle(tank));
    Frame+=3*TICKS_PER_SECOND-1;SiegeTank::AI(tank);
    Check("cannot complete conversion before three game seconds",*tank==UNIT_SIEGE_TANK&&SiegeTank::Transitioning(tank));
    ++Frame;SiegeTank::AI(tank);
    Check("conversion completes to the deployed class",*tank==UNIT_SIEGE_MODE&&!SiegeTank::Transitioning(tank));
}
bool Generate_Mission_Starts(const char* directory) {
    std::string dir(directory); auto tank=Tank(dir);
    auto mobile=&UnitTypeClass::As_Reference(UNIT_SIEGE_TANK);auto deployed=&UnitTypeClass::As_Reference(UNIT_SIEGE_MODE);
    Check("150 health and 10-tile sight",tank->Strength==150&&mobile->SightRange==10);
    Check("separate production type and unique INI identity",mobile->Type!=UNIT_MTANK&&UnitTypeClass::From_Name("SGTK")==UNIT_SIEGE_TANK);
    Check("new unit available to GDI and Nod",PlayerPtr->Can_Build(mobile,HOUSE_GOOD)&&HouseClass::As_Pointer(HOUSE_BAD)->Can_Build(mobile,HOUSE_BAD));
    Check("deployed form cannot be produced separately",!PlayerPtr->Can_Build(deployed,HOUSE_GOOD)&&!deployed->IsBuildable);
    Check("both native graphics and sidebar icon loaded",mobile->Get_Image_Data()&&deployed->Get_Image_Data()&&mobile->Get_Cameo_Data());
    Check("tank stats: 30 damage, seven tiles, native 23-tick reload",Weapons[mobile->Primary].Attack==30&&tank->Weapon_Range(0)==7*ICON_LEPTON_W&&tank->Rearm_Delay(false)==23);
    int chip=6; tank->Take_Damage(chip,0,WARHEAD_SA,nullptr);
    Check("base armor subtracts one damage point",tank->Strength==145);tank->Strength=150;
    Check("siege production takes approximately fifty game seconds",mobile->Time_To_Build(HOUSE_GOOD)==756);
    auto factory=new FactoryClass;Check("factory accepts Siege Tank production",factory->Set(*mobile,*PlayerPtr));
    factory->Force_Complete();Check("factory produces the new vehicle class",factory->Get_Object()&&SiegeTank::Is(factory->Get_Object()));delete factory;
    tank->Select();Check("clicking self exposes native deploy command",tank->What_Action(tank)==ACTION_SELF);
    auto target=Enemy(tank,5);auto id=tank->As_Target();auto targetID=target->As_Target();
    tank->SecondaryFacing.Set_Current(tank->Direction(target));tank->SecondaryFacing.Set_Desired(tank->Direction(target));tank->Arm=0;auto shell=static_cast<TechnoClass*>(tank)->Fire_At(targetID,0);Check("tank fires a dedicated visible shell",shell&&shell->Class->Type==BULLET_SIEGE_TANK&&shell->Strength==30);
    int health=target->Strength;for(int i=0;i<120;++i)TickBullets();
    Check("real tank projectile hits for 30 with no splash",target->Strength==health-30);
    Deploy(tank);Check("siege stats: 70 damage, twelve tiles, native 47-tick reload",Weapons[tank->Class->Primary].Attack==70&&tank->Weapon_Range(0)==12*ICON_LEPTON_W&&tank->Rearm_Delay(false)==47);
    auto position=tank->Coord;tank->Assign_Destination(::As_Target(Adjacent_Cell(Coord_Cell(position),FACING_S)));
    Check("siege mode refuses movement commands",!Target_Legal(tank->NavCom)&&!tank->Can_Player_Move());
    for(int i=0;i<50;++i){++Frame;tank->AI();}Check("deployed tank remains fixed through native AI",tank->Coord==position);
    auto close=Enemy(tank,1);tank->Arm=0;
    Check("minimum range rejects nearby target",tank->Can_Fire(close->As_Target(),0)==FIRE_CANT);
    tank->Assign_Target(close->As_Target());Frame=Frame/5*5;SiegeTank::AI(tank);
    Check("close enemy does not block farther target acquisition",tank->TarCom==targetID);
    auto aircraft=new AircraftClass(AIRCRAFT_ORCA,HOUSE_BAD);Put(aircraft,Coord_Cell(target->Coord));aircraft->Altitude=AircraftClass::FLIGHT_LEVEL;aircraft->IsDiscoveredByPlayer=true;
    Check("siege cannon cannot attack airborne aircraft",tank->Can_Fire(aircraft->As_Target(),0)!=FIRE_OK);
    for(int i=Bullets.Count()-1;i>=0;--i)delete Bullets.Ptr(i);tank->SecondaryFacing.Set_Current(tank->Direction(target));tank->SecondaryFacing.Set_Desired(tank->Direction(target));tank->Arm=0;shell=static_cast<TechnoClass*>(tank)->Fire_At(targetID,0);health=target->Strength;
    Check("siege fires its dedicated artillery shell",shell&&shell->Class->Type==BULLET_SIEGE_CANNON&&shell->Strength==70);
    for(int i=0;i<160;++i)TickBullets();std::printf("SIEGE DAMAGE %d\n",health-target->Strength);Check("real siege projectile hits for 70",target->Strength==health-70);
    Check("save deployed form",Save_Game((dir+"/deployed.sav").c_str(),"Siege deployed"));
    Check("reload deployed form",Load_Game((dir+"/deployed.sav").c_str()));tank=As_Unit(id);
    Check("deployed form survives save/reload",tank&&*tank==UNIT_SIEGE_MODE&&!tank->Can_Player_Move());
    auto damaged=tank->Strength=87;Check("start reverse conversion",SiegeTank::Toggle(tank));
    Check("save during conversion",Save_Game((dir+"/transition.sav").c_str(),"Siege transition"));
    Check("reload during conversion",Load_Game((dir+"/transition.sav").c_str()));tank=As_Unit(id);
    Check("conversion timer survives reload",tank&&SiegeTank::Transitioning(tank));
    Frame+=3*TICKS_PER_SECOND;SiegeTank::AI(tank);
    Check("reverse conversion preserves health and mobile class",*tank==UNIT_SIEGE_TANK&&tank->Strength==damaged&&tank->Can_Player_Move());
    tank->Assign_Destination(::As_Target(Adjacent_Cell(Coord_Cell(tank->Coord),FACING_S)));
    Check("mobile form accepts movement again",Target_Legal(tank->NavCom));
    // Ring damage at exact sub-cell distances, including allied collateral damage.
    tank=Tank(dir);target=Enemy(tank,6);auto origin=target->Coord;
    std::vector<UnitClass*> ring;
    for(int offset:{0,100,220,321}) {
        auto victim=new UnitClass(UNIT_MTANK,HOUSE_GOOD);Put(victim,Center());victim->Coord=XY_Coord(Coord_X(origin)+offset,Coord_Y(origin));victim->Strength=10000;ring.push_back(victim);
    }
    shell=new BulletClass(BULLET_SIEGE_CANNON);shell->Strength=70;shell->Coord=origin;shell->Payback=tank;
    Check("dedicated siege impact accepted",SiegeTank::Impact(shell,target->As_Target()));
    Check("three splash rings are 100, 50 and 25 percent; outside untouched",ring[0]->Strength==9930&&ring[1]->Strength==9965&&ring[2]->Strength==9983&&ring[3]->Strength==10000);
    Check("siege splash includes allied collateral damage",ring[0]->House==PlayerPtr&&ring[0]->Strength<10000);delete shell;
    tank=Tank(dir);auto second=new UnitClass(UNIT_SIEGE_TANK,HOUSE_GOOD);Put(second,XY_Cell(Cell_X(Center())+4,Cell_Y(Center())));Deploy(second);
    auto factoryBuilding=new BuildingClass(STRUCT_WEAP,HOUSE_GOOD);Put(factoryBuilding,XY_Cell(Cell_X(Center())-6,Cell_Y(Center())-4));
    for(int i=0;i<MAP_CELL_TOTAL;++i)if(Map.In_Radar(i))Map.Map_Cell(i,PlayerPtr,true);
    PlayerPtr->IsRecalcNeeded=true;PlayerPtr->AI();Map.Activate(1);tank->Select();
    ++ScenarioInit;Map.Set_Tactical_Position(Cell_Coord(XY_Cell(MAX(Map.MapCellX,Cell_X(Coord_Cell(tank->Coord))-8),MAX(Map.MapCellY,Cell_Y(Coord_Cell(tank->Coord))-6))));--ScenarioInit;
    Check("save isolated gameplay display fixture",Save_Game((dir+"/savegame.090").c_str(),"SIEGE TANK DISPLAY"));
    Check("current user save remains loadable",Load_Game((dir+"/current-user.sav").c_str()));
    // A new footer is optional when restoring older user games.
    Check("existing mission save remains loadable",Load_Game((dir+"/fixture.sav").c_str()));
    std::printf("TOTAL FAILURES %d\n",failures);return failures==0;
}
