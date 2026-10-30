// Paid campaign construction, wider factory exits and mobile forward bases.
// Orders live in native objects, so old and new saves need no extra footer/layout.
#include "function.h"
#include "campaignenemies.h"
#include "campaignexpansion.h"
#include "campaignformation.h"
#include <algorithm>
#include <queue>
#include <vector>
#include <limits>
namespace {
bool Live(TechnoClass const* b) { return b && b->IsActive && !b->IsInLimbo && b->Strength>0; }
std::vector<BuildingClass*> Bases(HouseClass const* h)
{
    std::vector<BuildingClass*> result;
    for(int i=0;i<Buildings.Count();++i){auto b=Buildings.Ptr(i);if(Live(b)&&b->House==h&&!b->Class->IsWall)result.push_back(b);}
    return result;
}
int CellDistance2(CELL a,CELL b){int x=Cell_X(a)-Cell_X(b),y=Cell_Y(a)-Cell_Y(b);return x*x+y*y;}
int FootprintDistance2(BuildingClass const* b,CELL c)
{
    int result=MAP_CELL_TOTAL*2;auto offsets=b->Occupy_List(true);
    for(;*offsets!=REFRESH_EOL;++offsets)result=std::min(result,CellDistance2(Coord_Cell(b->Coord)+*offsets,c));
    return result;
}
bool Near(std::vector<BuildingClass*> const& bases,CELL c,int radius)
{for(auto b:bases)if(FootprintDistance2(b,c)<=radius*radius)return true;return false;}
TechnoClass* Opponent(HouseClass const* h)
{
    TechnoClass* best=nullptr;int distance=std::numeric_limits<int>::max();auto bases=Bases(h);
    auto consider=[&](TechnoClass* t){if(!Live(t)||t->House!=PlayerPtr||!Map.In_Radar(Coord_Cell(t->Coord)))return;int d=bases.empty()?0:Distance(bases.front()->Coord,t->Coord);if(d<distance){distance=d;best=t;}};
    for(int i=0;i<Buildings.Count();++i)consider(Buildings.Ptr(i));
    if(!best)for(int i=0;i<Units.Count();++i)consider(Units.Ptr(i));
    if(!best)for(int i=0;i<Infantry.Count();++i)consider(Infantry.Ptr(i));
    return best;
}
bool Defense(StructType t){return t==STRUCT_GTOWER||t==STRUCT_ATOWER||t==STRUCT_TURRET||t==STRUCT_OBELISK||t==STRUCT_SAM;}
std::vector<CELL> Resources()
{
    std::vector<CELL> result;
    for(int y=Map.MapCellY;y<Map.MapCellY+Map.MapCellHeight;++y)
        for(int x=Map.MapCellX;x<Map.MapCellX+Map.MapCellWidth;++x){CELL c=XY_Cell(x,y);if(Map[c].Land_Type()==LAND_TIBERIUM)result.push_back(c);}
    return result;
}
// Keep production lanes open. Walls also leave three-cell-wide cardinal gates.
bool Space(BuildingTypeClass const& type,CELL cell,std::vector<BuildingClass*> const& bases)
{
    if(!type.Legal_Placement(cell)||!Near(bases,cell,10))return false;
    auto offsets=type.Occupy_List(true);
    for(auto b:bases){
        for(auto p=offsets;*p!=REFRESH_EOL;++p)
            if(FootprintDistance2(b,cell+*p)<=(type.IsWall?9:2))return false;
    }
    if(type.IsWall){
        auto anchor=*std::min_element(bases.begin(),bases.end(),[&](BuildingClass* a,BuildingClass* b){return FootprintDistance2(a,cell)<FootprintDistance2(b,cell);});
        CELL c=Coord_Cell(anchor->Center_Coord());int dx=ABS(Cell_X(c)-Cell_X(cell)),dy=ABS(Cell_Y(c)-Cell_Y(cell));
        if(dx<=1||dy<=1)return false;
        // Connected wall segments form perimeter corners, never an impassable closed ring.
        int ring=MAX(dx,dy);if(ring!=5&&ring!=6)return false;
    }
    return true;
}
CELL BuildingSite(HouseClass const* h,BuildingTypeClass const& type,BuildingClass const* producer=nullptr)
{
    auto bases=Bases(h);if(bases.empty())return 0;
    auto enemy=Opponent(h);auto resources=type.Type==STRUCT_REFINERY?Resources():std::vector<CELL>();
    std::vector<std::pair<long,CELL>> candidates;
    // At most one map sweep per completed building, not per simulation tick.
    for(int y=Map.MapCellY;y<Map.MapCellY+Map.MapCellHeight;++y)for(int x=Map.MapCellX;x<Map.MapCellX+Map.MapCellWidth;++x){
        CELL c=XY_Cell(x,y);if(!Space(type,c,bases))continue;
        int nearest=100000;for(auto b:bases)nearest=std::min(nearest,FootprintDistance2(b,c));
        long score=0;
        if(type.IsWall)score=-nearest;
        else if(Defense(type.Type))score=enemy?-CellDistance2(c,Coord_Cell(enemy->Coord)):nearest;
        else if(type.Type==STRUCT_REFINERY&&!resources.empty()){
            int tib=100000;for(CELL t:resources)tib=std::min(tib,CellDistance2(c,t));score=-8*tib-nearest;
        }else score=-ABS(nearest-25); // Grow with room around each facility.
        if(producer)score-=CellDistance2(c,Coord_Cell(producer->Center_Coord()))/4;
        candidates.emplace_back(score,c);
    }
    std::sort(candidates.begin(),candidates.end(),[](std::pair<long,CELL> a,std::pair<long,CELL> b){return a.first!=b.first?a.first>b.first:a.second<b.second;});
    for(auto candidate:candidates)if(Campaign_Construction_Preserves_Paths(type,candidate.second))return candidate.second;
    return 0;
}
void Dispatch(FootClass* f)
{
    if(!Campaign_Enemy_House(f->House))return;
    if(f->What_Am_I()==RTTI_UNIT){auto u=static_cast<UnitClass*>(f);if(u->Class->IsToHarvest){u->Assign_Mission(MISSION_HARVEST);u->Commence();return;}if(*u==UNIT_MCV)return;}
    if(f->Techno_Type_Class()->Primary==WEAPON_NONE||f->Techno_Type_Class()->IsTransporter)return;
    auto target=Opponent(f->House);if(!target)return;
    Campaign_Formation_Recruit(f);
}
int Count(std::vector<BuildingClass*> const& bases,StructType t){int n=0;for(auto b:bases)if(b->Class->Type==t)++n;return n;}
CELL ExpansionSite(UnitClass* mcv)
{
    auto bases=Bases(mcv->House);auto tib=Resources();auto target=Opponent(mcv->House);
    std::vector<unsigned char> visited(MAP_CELL_TOTAL,0);std::queue<CELL> queue;CELL start=Coord_Cell(mcv->Coord);queue.push(start);visited[start]=1;
    std::vector<std::pair<long,CELL>> candidates;
    auto const& yard=BuildingTypeClass::As_Reference(STRUCT_CONST);
    // Flood only traversable ground, so a distant site across water/cliffs is never selected.
    while(!queue.empty()){
        CELL c=queue.front();queue.pop();
        if(!Near(bases,c,15)){
            int x=Cell_X(c)-1,y=Cell_Y(c)-1;CELL top=XY_Cell(x,y);
            if(x>=Map.MapCellX&&y>=Map.MapCellY&&yard.Legal_Placement(top)){
                int nearTib=1000;for(CELL t:tib)nearTib=std::min(nearTib,CellDistance2(c,t));
                long score=-CellDistance2(start,c)-2*nearTib;
                if(target)score-=CellDistance2(c,Coord_Cell(target->Coord))/3;
                candidates.emplace_back(score,c);
            }
        }
        for(FacingType face=FACING_N;face<FACING_COUNT;face++){
            CELL next=Adjacent_Cell(c,face);if(next<0||next>=MAP_CELL_TOTAL||visited[next]||!Map.In_Radar(next))continue;
            if(mcv->Can_Enter_Cell(next)!=MOVE_OK)continue;
            // Diagonal movement may not pass through two touching solid obstacles.
            if(Cell_X(c)!=Cell_X(next)&&Cell_Y(c)!=Cell_Y(next)){
                if(mcv->Can_Enter_Cell(XY_Cell(Cell_X(c),Cell_Y(next)))!=MOVE_OK||mcv->Can_Enter_Cell(XY_Cell(Cell_X(next),Cell_Y(c)))!=MOVE_OK)continue;
            }
            visited[next]=1;queue.push(next);
        }
    }
    // Validate the most promising sites first, instead of flooding a local route
    // graph separately for every reachable map cell.
    std::stable_sort(candidates.begin(),candidates.end(),[](std::pair<long,CELL> a,std::pair<long,CELL> b){return a.first>b.first;});
    for(auto candidate:candidates){CELL c=candidate.second;CELL top=XY_Cell(Cell_X(c)-1,Cell_Y(c)-1);if(Campaign_Construction_Preserves_Paths(yard,top))return c;}
    return 0;
}
void Expand(UnitClass* mcv)
{
    if(!Live(mcv)||!Campaign_Enemy_House(mcv->House)||*mcv!=UNIT_MCV||mcv->Team||mcv->Trigger||mcv->In_Radio_Contact()||!Map.In_Radar(Coord_Cell(mcv->Coord)))return;
    if(mcv->IsDeploying){mcv->Try_To_Deploy();return;}
    // Native NavCom stores the route across save/load. Deploy only after actual arrival.
    if(Target_Legal(mcv->NavCom)||mcv->IsDriving||mcv->IsRotating)return;
    CELL c=Coord_Cell(mcv->Coord);auto bases=Bases(mcv->House);
    if(!Near(bases,c,12)){
        if(mcv->Try_To_Deploy())return; // May delete this MCV on success.
    }
    CELL goal=ExpansionSite(mcv);if(goal){mcv->Assign_Mission(MISSION_MOVE);mcv->Commence();mcv->Assign_Destination(::As_Target(goal));}
}
}
bool Campaign_Expansion_Near(HouseClass const* h,CELL cell)
{return Campaign_Enemy_House(h)&&Map.In_Radar(cell)&&Near(Bases(h),cell,10);}
TechnoTypeClass const* Campaign_Expansion_Suggest(HouseClass const* h,RTTIType kind)
{
    if(!Campaign_Enemy_House(h))return nullptr;
    auto bases=Bases(h);int yards=Count(bases,STRUCT_CONST);
    auto affordable=[&](TechnoTypeClass const& t){return h->Can_Build(&t,h->ActLike)&&t.Cost_Of()<=h->Available_Money();};
    if((kind==RTTI_UNIT||kind==RTTI_UNITTYPE)&&h->CurUnits<h->MaxUnit){
        int mcvs=0;for(int i=0;i<Units.Count();++i){auto u=Units.Ptr(i);if(u->IsActive&&u->House==h&&*u==UNIT_MCV)++mcvs;}
        auto& type=UnitTypeClass::As_Reference(UNIT_MCV);
        if(!mcvs&&yards<std::min(8,1+Frame/(5*60*TICKS_PER_SECOND))&&h->Available_Money()>=type.Cost_Of()+2000&&affordable(type))return &type;
    }
    if(kind!=RTTI_BUILDING&&kind!=RTTI_BUILDINGTYPE)return nullptr;
    if(!yards||h->CurBuildings>=h->MaxBuilding)return nullptr;
    bool nod=h->ActLike==HOUSE_BAD;
    auto select=[&](StructType t,int goal)->TechnoTypeClass const*{auto& b=BuildingTypeClass::As_Reference(t);return Count(bases,t)<goal&&affordable(b)?&b:nullptr;};
    TechnoTypeClass const* result;
    if((result=select(STRUCT_REFINERY,yards)))return result;
    if(h->Power<h->Drain+100){if((result=select(STRUCT_ADVANCED_POWER,100)))return result;if((result=select(STRUCT_POWER,100)))return result;}
    if((result=select(nod?STRUCT_HAND:STRUCT_BARRACKS,yards)))return result;
    if((result=select(nod?STRUCT_AIRSTRIP:STRUCT_WEAP,yards)))return result;
    if((result=select(STRUCT_REFINERY,2*yards)))return result;
    // Spread investment across production, heavy defenses and concrete perimeter walls.
    int walls=0;for(int y=Map.MapCellY;y<Map.MapCellY+Map.MapCellHeight;++y)for(int x=Map.MapCellX;x<Map.MapCellX+Map.MapCellWidth;++x){auto& c=Map[XY_Cell(x,y)];if(c.Overlay==OVERLAY_BRICK_WALL&&c.Owner==h->Class->House)++walls;}
    struct Goal{StructType type;int count;};
    Goal goals[]={{nod?STRUCT_TURRET:STRUCT_GTOWER,6*yards},{nod?STRUCT_OBELISK:STRUCT_ATOWER,4*yards},{STRUCT_SAM,3*yards},{nod?STRUCT_HAND:STRUCT_BARRACKS,2*yards},{nod?STRUCT_AIRSTRIP:STRUCT_WEAP,2*yards},{STRUCT_REFINERY,3*yards}};
    // Lowest completion fraction wins. Walls get an equal recurring share, not an endless priority.
    int fraction=100000;StructType choice=STRUCT_NONE;
    for(auto goal:goals){auto& b=BuildingTypeClass::As_Reference(goal.type);int f=1000*Count(bases,goal.type)/goal.count;if(f<fraction&&f<1000&&affordable(b)){fraction=f;choice=goal.type;}}
    auto& wall=BuildingTypeClass::As_Reference(STRUCT_BRICK_WALL);
    if(1000*walls/(32*yards)<fraction&&walls<32*yards&&affordable(wall)&&BuildingSite(h,wall))return &wall;
    return choice==STRUCT_NONE?nullptr:&BuildingTypeClass::As_Reference(choice);
}
int Campaign_Expansion_Exit(BuildingClass* producer,TechnoClass* product)
{
    if(GameToPlay!=GAME_NORMAL||!producer||!product)return -1;
    if(product->What_Am_I()==RTTI_BUILDING&&Campaign_Enemy_House(producer->House)){
        auto type=static_cast<BuildingClass*>(product)->Class;CELL cell=BuildingSite(producer->House,*type,producer);
        if(!cell)return 1; // Retain the paid product, retry when terrain/units change.
        // Wall Unlimbo converts the object to an overlay and deletes it. Do not dereference it afterwards.
        return product->Unlimbo(Cell_Coord(cell))?2:1;
    }
    auto kind=product->What_Am_I();
    if(kind!=RTTI_UNIT&&kind!=RTTI_INFANTRY&&kind!=RTTI_AIRCRAFT)return -1;
    RTTIType expected=kind==RTTI_UNIT?RTTI_UNITTYPE:kind==RTTI_INFANTRY?RTTI_INFANTRYTYPE:RTTI_AIRCRAFTTYPE;
    if(producer->Class->ToBuild!=expected)return -1;
    if(kind==RTTI_AIRCRAFT&&!Campaign_Enemy_House(producer->House))return -1;
    if(kind==RTTI_AIRCRAFT)static_cast<AircraftClass*>(product)->Altitude=AircraftClass::FLIGHT_LEVEL;
    auto foot=static_cast<FootClass*>(product);CELL center=Coord_Cell(producer->Center_Coord());
    for(int radius=1;radius<=12;++radius)for(int dy=-radius;dy<=radius;++dy)for(int dx=-radius;dx<=radius;++dx){
        if(MAX(ABS(dx),ABS(dy))!=radius)continue;int x=Cell_X(center)+dx,y=Cell_Y(center)+dy;
        if(x<0||x>=MAP_CELL_W||y<0||y>=MAP_CELL_H)continue;CELL cell=XY_Cell(x,y);
        if(!Map.In_Radar(cell)||foot->Can_Enter_Cell(cell)!=MOVE_OK)continue;
        if(foot->Unlimbo(Cell_Coord(cell),DIR_SW)){foot->Assign_Mission(MISSION_GUARD);foot->Commence();Dispatch(foot);return 2;}
    }
    return 1;
}
void Campaign_Expansion_AI()
{
    if(GameToPlay!=GAME_NORMAL||!GameActive||ScenarioInit||!PlayerPtr||!PlayerPtr->IsHuman||PlayerWins||PlayerLoses)return;
    for(int i=0;i<Houses.Count();++i){auto h=Houses.Ptr(i);if(Campaign_Enemy_House(h)&&!Bases(h).empty()){h->IsStarted=true;h->IsAlerted=true;}}
    std::vector<TARGET> mcvs;for(int i=0;i<Units.Count();++i)if(*Units.Ptr(i)==UNIT_MCV)mcvs.push_back(Units.Ptr(i)->As_Target());
    for(auto id:mcvs){auto t=As_Techno(id);if(t&&t->What_Am_I()==RTTI_UNIT)Expand(static_cast<UnitClass*>(t));}
}
void Campaign_Expansion_Resources()
{
    // Five times the previous four-step campaign setting. Native growth/spread rules stay intact.
    int steps=GameToPlay==GAME_NORMAL?20:1;for(int i=0;i<steps;++i)Map.Logic();
}
