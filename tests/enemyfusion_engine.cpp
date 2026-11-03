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
    Check("first evolution has 1.5x size with twofold health and power",EnemyFusion::Tier(evolved)==1&&EnemyFusion::Multiplier(evolved)==2&&EnemyFusion::Visual_Scale(evolved)==384&&Campaign_Max_Health(evolved)==base*2&&evolved->Strength==base*2&&EnemyFusion::Firepower(evolved,30)==60);
    Check("fusion preserves native class owner and mission",&evolved->Class_Of()==&anchor->Class_Of()&&evolved->House==house&&evolved->Mission==MISSION_SLEEP);
    Check("fusion releases nine heap slots without combat losses",(kind==RTTI_UNIT?Units.Count():kind==RTTI_INFANTRY?Infantry.Count():Aircraft.Count())==before-9&&house->UnitsLost==lost&&PlayerPtr->UnitsKilled[HOUSE_BAD]==killed);
    Check("full upgraded health uses the correct health bar ratio",evolved->Health_Ratio()==0x100);
    auto id=evolved->As_Target();Check("save upgraded actor",Save_Game((dir+"/fusion.sav").c_str(),"Fusion stage one"));
    Check("load upgraded actor",Load_Game((dir+"/fusion.sav").c_str()));evolved=static_cast<FootClass*>(As_Techno(id));
    Check("save reload preserves tier health and size without multiplying again",evolved&&EnemyFusion::Tier(evolved)==1&&evolved->Strength==base*2&&Campaign_Max_Health(evolved)==base*2);
    std::vector<TARGET> tierOne{id};
    for(int n=0;n<9;++n){auto more=Group(kind);EnemyFusion::AI();auto one=Fused(more);if(one)tierOne.push_back(one->As_Target());}
    EnemyFusion::AI();auto twice=Fused(tierOne);if(!twice)return;
    Check("second evolution has 2.25x size with fourfold health and power",EnemyFusion::Tier(twice)==2&&EnemyFusion::Multiplier(twice)==4&&EnemyFusion::Visual_Scale(twice)==576&&Campaign_Max_Health(twice)==base*4&&twice->Strength==base*4&&EnemyFusion::Firepower(twice,30)==120);
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
    Reset(dir);ids=Group(RTTI_UNIT,9);auto distant=Spawn(RTTI_UNIT,HOUSE_BAD,0,XY_Cell(Cell_X(Center())+20,Cell_Y(Center())));EnemyFusion::AI();
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
static FootClass* BuildTier(int tier) {
    if(!tier)return Spawn(RTTI_UNIT);
    std::vector<TARGET> ids;
    for(int i=0;i<10;++i){auto child=BuildTier(tier-1);if(!child)return nullptr;ids.push_back(child->As_Target());}
    EnemyFusion::AI();return Fused(ids);
}
static void CountsAndRadius(const std::string& dir) {
    Reset(dir);Check("initial empty field has zero unit counts",EnemyFusion::Count_Units()==EnemyFusion::Counts{});
    auto ids=Group(RTTI_UNIT,9);auto counts=EnemyFusion::Count_Units();Check("normal deployed actors counted individually",counts[0]==9&&counts[1]==0);
    auto tenth=Spawn(RTTI_UNIT);ids.push_back(tenth->As_Target());EnemyFusion::AI();auto one=Fused(ids);
    counts=EnemyFusion::Count_Units();Check("fusion updates normal and upgraded counts",counts[0]==0&&counts[1]==1);
    auto own=Spawn(RTTI_UNIT,HOUSE_GOOD);auto neutral=Spawn(RTTI_INFANTRY,HOUSE_NEUTRAL);auto air=Spawn(RTTI_AIRCRAFT);auto infantry=Spawn(RTTI_INFANTRY);
    auto building=new BuildingClass(STRUCT_POWER,HOUSE_BAD);Put(building,XY_Cell(Cell_X(Center())-9,Cell_Y(Center())-9));
    auto product=new UnitClass(UNIT_MTANK,HOUSE_BAD);Campaign_Enemies_Mark_Extra(product);
    counts=EnemyFusion::Count_Units();Check("counts include field infantry and aircraft but exclude buildings player neutral and limbo production",counts[0]==2&&counts[1]==1);
    air->Limbo();infantry->Strength=0;counts=EnemyFusion::Count_Units();Check("counts immediately exclude limbo and dead actors",counts[0]==0&&counts[1]==1);
    if(one){one->Stun();delete one;}Check("deleting an upgraded actor clears its count",EnemyFusion::Count_Units()[1]==0);
    Reset(dir);ids=Group(RTTI_UNIT,9);auto c=Center();
    // Place all donors to the west of the first anchor so a closer alternative anchor cannot defeat the boundary test.
    for(auto id:ids)As_Unit(id)->Limbo();
    for(unsigned i=0;i<ids.size();++i){auto o=As_Unit(ids[i]);Check("place radius boundary donor",o->Unlimbo(Cell_Coord(XY_Cell(Cell_X(c)-int(i),Cell_Y(c))),DIR_N));}
    tenth=Spawn(RTTI_UNIT,HOUSE_BAD,0,XY_Cell(Cell_X(c)+10,Cell_Y(c)));ids.push_back(tenth->As_Target());tenth->Coord=XY_Coord(Coord_X(As_Techno(ids.front())->Coord)+10*ICON_LEPTON_W,Coord_Y(As_Techno(ids.front())->Coord));
    Check("exact native distance at ten cells",::Distance(tenth->Coord,As_Techno(ids.front())->Coord)==10*ICON_LEPTON_W);
    EnemyFusion::AI();Check("ten-cell edge includes the tenth actor",EnemyFusion::Tier(As_Techno(ids.front()))==1);
    Reset(dir);ids=Group(RTTI_UNIT,9);
    for(auto id:ids)As_Unit(id)->Limbo();
    for(unsigned i=0;i<ids.size();++i){auto o=As_Unit(ids[i]);o->Unlimbo(Cell_Coord(XY_Cell(Cell_X(c)-int(i),Cell_Y(c))),DIR_N);}
    tenth=Spawn(RTTI_UNIT,HOUSE_BAD,0,XY_Cell(Cell_X(c)+10,Cell_Y(c)));tenth->Coord=XY_Coord(Coord_X(As_Techno(ids.front())->Coord)+10*ICON_LEPTON_W+1,Coord_Y(As_Techno(ids.front())->Coord));
    Check("exact native distance outside ten cells",::Distance(tenth->Coord,As_Techno(ids.front())->Coord)==10*ICON_LEPTON_W+1);
    EnemyFusion::AI();Check("ten cells plus one lepton remains outside fusion range",EnemyFusion::Tier(As_Techno(ids.front()))==0&&As_Techno(ids.back()));
}
static void Screen(const std::string& dir) {
    Reset(dir);auto third=BuildTier(3);auto second=BuildTier(2);auto first=BuildTier(1);auto normal=BuildTier(0);
    if(!third||!second||!first||!normal)return;
    Check("third evolution separates 3.375x size from 8x combat stats",EnemyFusion::Tier(third)==3&&EnemyFusion::Visual_Scale(third)==864&&EnemyFusion::Firepower(third,30)==240&&Campaign_Max_Health(third)==third->Class_Of().MaxStrength*8);
    std::vector<FootClass*> actors{normal,first,second,third};
    for(auto actor:actors)actor->Limbo();auto c=Center();
    for(unsigned i=0;i<actors.size();++i){auto actor=actors[i];Check("place rendered tier",actor->Unlimbo(Cell_Coord(XY_Cell(Cell_X(c)-6+int(i)*5,Cell_Y(c))),DIR_N));actor->Assign_Mission(MISSION_SLEEP);actor->Commence();}
    auto counts=EnemyFusion::Count_Units();Check("HUD counts all four rendered stages independently",counts[0]==1&&counts[1]==1&&counts[2]==1&&counts[3]==1);
    auto shape=normal->Class_Of().Get_Image_Data();int sw=Get_Build_Frame_Width(shape),sh=Get_Build_Frame_Height(shape);
    auto built=Build_Frame(shape,0,_ShapeBuffer);auto raw=static_cast<unsigned char*>(Get_Shape_Header_Data(reinterpret_cast<void*>(built)));
    std::vector<unsigned char> original(raw,raw+sw*sh);int scales[4]={256,384,576,864};
    GraphicBufferClass raster(960,600,nullptr,960*600);auto previous=Set_Logic_Page(raster);auto cacheMode=UseBigShapeBuffer;
    for(unsigned tier=0;tier<actors.size();++tier){
        raster.Clear();CC_Draw_Shape(actors[tier],shape,0,150,150,WINDOW_TACTICAL,SHAPE_CENTER|SHAPE_WIN_REL);
        auto bytes=static_cast<unsigned char*>(raster.Get_Buffer());int w=(sw*scales[tier]+128)/256,h=(sh*scales[tier]+128)/256;
        bool exact=true;for(int y=0;y<h;++y)for(int x=0;x<w;++x)if(bytes[(150+WindowList[WINDOW_TACTICAL][WINDOWY]-h/2+y)*960+150+WindowList[WINDOW_TACTICAL][WINDOWX]-w/2+x]!=original[(y*sh/h)*sw+x*sw/w])exact=false;
        Check("actual raster retains exact original palette pixels at the expected 1.5-power size",exact&&EnemyFusion::Visual_Scale(actors[tier])==scales[tier]);
        Check("sprite enlargement restores native cache mode",UseBigShapeBuffer==cacheMode);
    }
    raster.Clear();EnemyFusion::Draw_Counts();
    auto screenBytes=static_cast<unsigned char*>(raster.Get_Buffer());std::vector<unsigned char> hudBefore(screenBytes,screenBytes+960*600);
    auto normalCoord=normal->Coord;normal->Limbo();EnemyFusion::Draw_Counts();bool updated=false;
    for(int y=WindowList[WINDOW_TACTICAL][WINDOWY]+6;y<WindowList[WINDOW_TACTICAL][WINDOWY]+78;++y)
        for(int x=WindowList[WINDOW_TACTICAL][WINDOWX]+6;x<WindowList[WINDOW_TACTICAL][WINDOWX]+136;++x)
            if(screenBytes[y*960+x]!=hudBefore[y*960+x])updated=true;
    Check("rendered counter changes after an actor leaves the field",updated&&EnemyFusion::Count_Units()[0]==0);
    Check("restore displayed normal actor",normal->Unlimbo(normalCoord,DIR_N));
    Set_Logic_Page(previous);
    for(int i=0;i<MAP_CELL_TOTAL;++i)if(Map.In_Radar(i))Map.Map_Cell(i,PlayerPtr,true);
    ++ScenarioInit;Map.Set_Tactical_Position(Cell_Coord(XY_Cell(Cell_X(c)-11,Cell_Y(c)-8)));--ScenarioInit;
    Special.HealthBarDisplayMode=SpecialClass::HB_ALWAYS;PlayerPtr->IsRecalcNeeded=true;Map.Activate(1);Map.Flag_To_Redraw(true);
    auto old=Set_Logic_Page(HidPage);Map.Draw_It(true);Set_Logic_Page(old);
    GraphicBufferClass image(HidPage.Get_Width(),HidPage.Get_Height(),nullptr,HidPage.Get_Width()*HidPage.Get_Height());HidPage.Blit(image);
    auto path=dir+"/fusion-hud-gameplay.pcx";Write_PCX_File(const_cast<char*>(path.c_str()),image,GamePalette);CCFileClass capture(path.c_str());Check("capture native gameplay with count HUD",capture.Is_Available()&&capture.Size()>1000);
    Check("save visual validation fixture",Save_Game((dir+"/savegame.090").c_str(),"Enemy fusion stages and counts"));
    auto id=third->As_Target();Check("reload third-tier save and count HUD",Load_Game((dir+"/savegame.090").c_str()));
    Check("third tier and counts persist with new visual scaling",EnemyFusion::Tier(As_Techno(id))==3&&EnemyFusion::Visual_Scale(As_Techno(id))==864&&EnemyFusion::Count_Units()[3]==1);
}
static void LegacyAndBadFooter(const std::string& dir) {
    Check("previous 2x-size fusion save still loads",Load_Game((dir+"/previous-fusion.sav").c_str()));
    bool migrated=false;for(int i=0;i<Aircraft.Count();++i)if(EnemyFusion::Tier(Aircraft.Ptr(i))==2)migrated=EnemyFusion::Visual_Scale(Aircraft.Ptr(i))==576&&EnemyFusion::Multiplier(Aircraft.Ptr(i))==4;
    Check("old saved tier changes only its visual size and preserves fourfold power",migrated&&EnemyFusion::Count_Units()[2]==1);
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
    ScopeTests(dir);NativeTickAndMixedTier(dir);ShotTest(dir);CountsAndRadius(dir);Screen(dir);LegacyAndBadFooter(dir);
    std::printf("TOTAL FAILURES %d\n",failures);return failures==0;
}
