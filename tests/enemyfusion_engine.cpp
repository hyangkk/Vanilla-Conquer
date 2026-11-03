// Real engine integration test; run only through the private mission-start probe.
#define Generate_Mission_Starts SAM_Regression
#include "../tests/samblast_engine.cpp"
#undef Generate_Mission_Starts
#include "enemyfusion.h"
#include "common/filepcx.h"
#include "common/keyframe.h"
#include "common/renderlayers.h"
static FootClass* Spawn(RTTIType kind,HousesType owner=HOUSE_BAD,int variant=0,CELL cell=0) {
    FootClass* o=kind==RTTI_UNIT?static_cast<FootClass*>(new UnitClass(variant?UNIT_JEEP:UNIT_MTANK,owner)):
        kind==RTTI_INFANTRY?static_cast<FootClass*>(new InfantryClass(variant?INFANTRY_E2:INFANTRY_E1,owner)):
        static_cast<FootClass*>(new AircraftClass(variant?AIRCRAFT_HELICOPTER:AIRCRAFT_ORCA,owner));
    Check("place fusion actor",o&&Put(o,cell?cell:Center()));
    o->Assign_Mission(MISSION_SLEEP);o->Commence();o->IsDiscoveredByPlayer=true;
    Campaign_Enemies_Mark_Extra(o);return o;
}
static std::vector<TARGET> Group(RTTIType kind,int count=10,HousesType owner=HOUSE_BAD) {
    std::vector<TARGET> ids;for(int i=0;i<count;++i)ids.push_back(Spawn(kind,owner)->As_Target());return ids;
}
static FootClass* Fused(const std::vector<TARGET>& ids) {
    FootClass* result=nullptr;int alive=0;for(auto id:ids)if(auto o=As_Techno(id)){++alive;result=static_cast<FootClass*>(o);}
    Check("ten actors leave exactly one survivor",alive==1);return alive==1?result:nullptr;
}
static void Evolution(const std::string& dir,RTTIType kind) {
    Reset(dir);auto ids=Group(kind,9);EnemyFusion::AI();int count=0;for(auto id:ids)if(As_Techno(id))++count;
    Check("nine same-tier actors cannot fuse",count==9&&EnemyFusion::Tier(As_Techno(ids[0]))==0);
    ids.push_back(Spawn(kind)->As_Target());auto anchor=As_Techno(ids[0]);int base=anchor->Class_Of().MaxStrength;
    auto house=anchor->House;unsigned lost=house->UnitsLost,killed=PlayerPtr->UnitsKilled[HOUSE_BAD];int before=kind==RTTI_UNIT?Units.Count():kind==RTTI_INFANTRY?Infantry.Count():Aircraft.Count();
    EnemyFusion::AI();auto evolved=Fused(ids);if(!evolved)return;
    Check("first evolution has twofold size health and power",EnemyFusion::Tier(evolved)==1&&EnemyFusion::Multiplier(evolved)==2&&Campaign_Max_Health(evolved)==base*2&&evolved->Strength==base*2&&EnemyFusion::Firepower(evolved,30)==60);
    Check("fusion preserves native class owner and mission",&evolved->Class_Of()==&anchor->Class_Of()&&evolved->House==house&&evolved->Mission==MISSION_SLEEP);
    Check("fusion releases nine heap slots without combat losses",(kind==RTTI_UNIT?Units.Count():kind==RTTI_INFANTRY?Infantry.Count():Aircraft.Count())==before-9&&house->UnitsLost==lost&&PlayerPtr->UnitsKilled[HOUSE_BAD]==killed);
    Check("full upgraded health uses the correct health bar ratio",evolved->Health_Ratio()==0x100);
    auto id=evolved->As_Target();Check("save upgraded actor",Save_Game((dir+"/fusion.sav").c_str(),"Fusion stage one"));
    Check("load upgraded actor",Load_Game((dir+"/fusion.sav").c_str()));evolved=static_cast<FootClass*>(As_Techno(id));
    Check("save reload preserves tier health and size without multiplying again",evolved&&EnemyFusion::Tier(evolved)==1&&evolved->Strength==base*2&&Campaign_Max_Health(evolved)==base*2);
    std::vector<TARGET> tierOne{id};
    for(int n=0;n<9;++n){auto more=Group(kind);EnemyFusion::AI();auto one=Fused(more);if(one)tierOne.push_back(one->As_Target());}
    EnemyFusion::AI();auto twice=Fused(tierOne);if(!twice)return;
    Check("ten upgraded actors recursively evolve to fourfold size health and power",EnemyFusion::Tier(twice)==2&&EnemyFusion::Multiplier(twice)==4&&Campaign_Max_Health(twice)==base*4&&twice->Strength==base*4&&EnemyFusion::Firepower(twice,30)==120);
    id=twice->As_Target();Check("save second evolution",Save_Game((dir+"/fusion-two.sav").c_str(),"Fusion stage two"));
    Check("reload second evolution",Load_Game((dir+"/fusion-two.sav").c_str()));
    Check("recursive tier survives second save",As_Techno(id)&&EnemyFusion::Tier(As_Techno(id))==2);
    auto old=As_Techno(id);old->Stun();delete old;auto reuse=Spawn(kind);
    Check("reused heap slot cannot inherit an old upgrade",EnemyFusion::Tier(reuse)==0&&Campaign_Max_Health(reuse)==reuse->Class_Of().MaxStrength);
}
static void ScopeTests(const std::string& dir) {
    Reset(dir);auto own=Group(RTTI_UNIT,10,HOUSE_GOOD);auto neutral=Group(RTTI_INFANTRY,10,HOUSE_NEUTRAL);
    EnemyFusion::AI();Check("player and neutral actors never fuse",As_Techno(own[9])&&As_Techno(neutral[9])&&EnemyFusion::Tier(As_Techno(own[0]))==0);
    Reset(dir);auto ids=Group(RTTI_UNIT,9);auto other=Spawn(RTTI_UNIT,HOUSE_BAD,1);EnemyFusion::AI();
    Check("different native unit types never mix",As_Techno(ids[9-1])&&EnemyFusion::Tier(other)==0);
    Reset(dir);ids=Group(RTTI_UNIT);GameToPlay=GAME_SKIRMISH;EnemyFusion::AI();Check("skirmish is unchanged",As_Techno(ids[9])&&EnemyFusion::Tier(As_Techno(ids[0]))==0);GameToPlay=GAME_NORMAL;
    Reset(dir);ids=Group(RTTI_UNIT,9);auto distant=Spawn(RTTI_UNIT,HOUSE_BAD,0,XY_Cell(Cell_X(Center())+12,Cell_Y(Center())));EnemyFusion::AI();
    Check("distant tenth actor does not trigger global fusion",As_Techno(ids[8])&&EnemyFusion::Tier(distant)==0);
    Reset(dir);ids=Group(RTTI_UNIT);auto tether=As_Techno(ids.back());tether->IsTethered=true;EnemyFusion::AI();
    Check("factory tether defers fusion safely",As_Techno(ids.back())&&EnemyFusion::Tier(tether)==0);tether->IsTethered=false;EnemyFusion::AI();Fused(ids);
    Reset(dir);ids=Group(RTTI_UNIT);for(auto id:ids)As_Techno(id)->Strength/=2;EnemyFusion::AI();auto o=Fused(ids);
    Check("fusion averages damage instead of fully healing",o&&o->Strength==o->Class_Of().MaxStrength&&o->Health_Ratio()==0x80);
    Reset(dir);ids=Group(RTTI_UNIT);auto bad=HouseClass::As_Pointer(HOUSE_BAD);bad->Make_Ally(HOUSE_GOOD);PlayerPtr->Make_Ally(HOUSE_BAD);EnemyFusion::AI();
    Check("allied AI actors are excluded",As_Techno(ids[9])&&EnemyFusion::Tier(As_Techno(ids[0]))==0);bad->Make_Enemy(HOUSE_GOOD);PlayerPtr->Make_Enemy(HOUSE_BAD);
}
static void NativeTickAndMixedTier(const std::string& dir) {
    Reset(dir);auto ids=Group(RTTI_UNIT);Frame=TICKS_PER_SECOND;Logic.AI();auto one=Fused(ids);
    Check("normal Logic AI invokes fusion without a probe-only shortcut",one&&EnemyFusion::Tier(one)==1);
    auto lower=Group(RTTI_UNIT,9);EnemyFusion::AI();
    Check("one upgraded plus nine base actors cannot mix tiers",As_Techno(lower.back())&&one&&EnemyFusion::Tier(one)==1);
    if(one){one->Stun();delete one;}Reset(dir);ids=Group(RTTI_UNIT);
    std::vector<CELL> occupied;for(auto id:ids)occupied.push_back(Coord_Cell(As_Techno(id)->Coord));
    EnemyFusion::AI();one=Fused(ids);bool free=true;
    auto checker=new UnitClass(UNIT_MTANK,HOUSE_BAD);
    for(auto cell:occupied)if(!one||cell!=Coord_Cell(one->Coord))free=free&&checker->Can_Enter_Cell(cell)==MOVE_OK;
    Check("merged donors release their occupied map cells",free);delete checker;
    Reset(dir);ids=Group(RTTI_UNIT);auto trigger=new TriggerClass;trigger->Action=TriggerClass::ACTION_NONE;
    for(auto id:ids)As_Techno(id)->Trigger=trigger;EnemyFusion::AI();
    Check("mission-linked donors are never silently removed",As_Techno(ids.back())&&EnemyFusion::Tier(As_Techno(ids.front()))==0);
    for(auto id:ids)As_Techno(id)->Trigger=nullptr;delete trigger;
}
static void ShotTest(const std::string& dir) {
    Reset(dir);auto ids=Group(RTTI_UNIT);EnemyFusion::AI();auto o=Fused(ids);if(!o)return;
    auto target=Spawn(RTTI_UNIT,HOUSE_GOOD,0,XY_Cell(Cell_X(Coord_Cell(o->Coord))+3,Cell_Y(Coord_Cell(o->Coord))));
    auto unit=static_cast<UnitClass*>(o);unit->SecondaryFacing.Set_Current(unit->Direction(target));unit->SecondaryFacing.Set_Desired(unit->Direction(target));unit->Arm=0;
    auto weapon=unit->Techno_Type_Class()->Primary;auto shot=unit->TechnoClass::Fire_At(target->As_Target(),0);
    Check("real projectile carries twice the original weapon damage",shot&&shot->Strength==2*Weapons[weapon].Attack);
    Check("range and reload remain native",unit->Weapon_Range(0)==Weapons[weapon].Range&&EnemyFusion::Firepower(target,30)==30);
}
static void Screen(const std::string& dir) {
    Reset(dir);std::vector<TARGET> stageOne;
    for(int n=0;n<10;++n){auto ids=Group(RTTI_UNIT);EnemyFusion::AI();auto o=Fused(ids);if(o)stageOne.push_back(o->As_Target());}
    EnemyFusion::AI();auto giant=Fused(stageOne);if(!giant)return;
    auto mediumIDs=Group(RTTI_UNIT);EnemyFusion::AI();auto medium=Fused(mediumIDs);auto normal=Spawn(RTTI_UNIT);
    normal->Limbo();medium->Limbo();giant->Limbo();
    auto center=Center();auto relocate=[&](FootClass* o,int dx){o->Limbo();Check("place rendered tier",o->Unlimbo(Cell_Coord(XY_Cell(Cell_X(center)+dx,Cell_Y(center))),DIR_N));o->Assign_Mission(MISSION_SLEEP);o->Commence();};
    relocate(normal,-5);relocate(medium,0);relocate(giant,6);
    for(int i=0;i<MAP_CELL_TOTAL;++i)if(Map.In_Radar(i))Map.Map_Cell(i,PlayerPtr,true);
    ++ScenarioInit;Map.Set_Tactical_Position(Cell_Coord(XY_Cell(Cell_X(center)-11,Cell_Y(center)-8)));--ScenarioInit;
    GraphicBufferClass raster(960,600,nullptr,960*600);
    auto previous=Set_Logic_Page(raster);std::vector<int> coverage;
    auto cacheMode=UseBigShapeBuffer;
    for(auto actor:{normal,medium,giant}) {
        raster.Clear();CC_Draw_Shape(actor,actor->Class_Of().Get_Image_Data(),0,150,150,WINDOW_TACTICAL,SHAPE_CENTER|SHAPE_WIN_REL);
        auto bytes=static_cast<unsigned char*>(raster.Get_Buffer());int opaque=0;
        for(int i=0;i<960*600;++i)if(bytes[i])++opaque;coverage.push_back(opaque);
        Check("sprite enlargement restores native cache mode",UseBigShapeBuffer==cacheMode);
    }
    Set_Logic_Page(previous);
    Check("actual raster preserves artwork and doubles both dimensions per tier",coverage[0]>0&&coverage[1]==coverage[0]*4&&coverage[2]==coverage[0]*16);
    Special.HealthBarDisplayMode=SpecialClass::HB_ALWAYS;
    PlayerPtr->IsRecalcNeeded=true;Map.Activate(1);Map.Flag_To_Redraw(true);
    auto old=Set_Logic_Page(HidPage);Map.Draw_It(true);Set_Logic_Page(old);
    GraphicBufferClass image(HidPage.Get_Width(),HidPage.Get_Height(),nullptr,HidPage.Get_Width()*HidPage.Get_Height());HidPage.Blit(image);
    auto path=dir+"/fusion-gameplay.pcx";Write_PCX_File(const_cast<char*>(path.c_str()),image,GamePalette);CCFileClass capture(path.c_str());Check("capture actual native gameplay render",capture.Is_Available()&&capture.Size()>1000);
    Check("save visual validation fixture",Save_Game((dir+"/savegame.090").c_str(),"Enemy fusion 1x 2x 4x"));
}
static void LegacyAndBadFooter(const std::string& dir) {
    Check("current user save still loads",Load_Game((dir+"/current-user.sav").c_str()));
    Check("pre-fusion mission saves still load",Load_Game((dir+"/fixture.sav").c_str()));
    Check("old save starts with no arbitrary upgrades",EnemyFusion::Tier(Units.Count()?Units.Ptr(0):nullptr)==0);
    Reset(dir);auto ids=Group(RTTI_UNIT);EnemyFusion::AI();Fused(ids);Check("write corrupt footer fixture",Save_Game((dir+"/invalid.sav").c_str(),"Invalid fusion"));
    {std::ifstream in(dir+"/invalid.sav",std::ios::binary);std::string b((std::istreambuf_iterator<char>(in)),{});auto p=b.rfind("CNCFUS01");Check("optional versioned fusion footer exists",p!=std::string::npos);if(p==std::string::npos)return;std::uint32_t invalid=16;std::memcpy(&b[p+16],&invalid,4);std::ofstream out(dir+"/invalid.sav",std::ios::binary|std::ios::trunc);out.write(b.data(),b.size());}
    Check("out-of-range tier rejects without decoding invalid pointers",!Load_Game((dir+"/invalid.sav").c_str()));
    {std::ifstream in(dir+"/invalid.sav",std::ios::binary);std::string b((std::istreambuf_iterator<char>(in)),{});auto p=b.rfind("CNCFUS01");std::uint32_t cap=15;std::memcpy(&b[p+16],&cap,4);std::ofstream out(dir+"/bounded.sav",std::ios::binary|std::ios::trunc);out.write(b.data(),b.size());}
    Check("highest supported tier loads safely",Load_Game((dir+"/bounded.sav").c_str()));
    auto capped=As_Techno(ids.front());Check("native short health and firepower saturate instead of overflowing",capped&&Campaign_Max_Health(capped)==32767&&EnemyFusion::Firepower(capped,300)==32767);

}
bool Generate_Mission_Starts(const char* directory) {
    std::string dir(directory);for(auto kind:{RTTI_UNIT,RTTI_INFANTRY,RTTI_AIRCRAFT})Evolution(dir,kind);
    ScopeTests(dir);NativeTickAndMixedTier(dir);ShotTest(dir);Screen(dir);LegacyAndBadFooter(dir);
    std::printf("TOTAL FAILURES %d\n",failures);return failures==0;
}
