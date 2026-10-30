// Paid automatic field repair for all player campaign forces.
#include "function.h"
#include "campaigndefense.h"
#include "campaignrepair.h"
#include <cstdint>
void Campaign_Auto_Repair(TechnoClass* object)
{
    if(!GameActive||ScenarioInit||Frame%TICKS_PER_SECOND||!Campaign_Player_Repair(object)
        ||!object->IsActive||object->IsInLimbo||object->Strength<=0)return;
    auto kind=object->What_Am_I();
    // Buildings keep their original repair animation, cost, and timing in Building::AI.
    if(kind!=RTTI_UNIT&&kind!=RTTI_INFANTRY&&kind!=RTTI_AIRCRAFT)return;
    int maximum=Campaign_Max_Health(object);if(object->Strength>=maximum)return;
    int step=MAX(1,maximum/50); // Two percent per game second, with at least one HP.
    int restored=MIN(step,maximum-object->Strength);
    auto type=object->Techno_Type_Class();
    int cost=MAX(1,int(std::int64_t(type->Cost_Of())*restored/(MAX(1,int(type->MaxStrength))*4)));
    if(object->House->Available_Money()<cost)return;
    object->House->Spend_Money(cost);object->Strength+=restored;object->Mark(MARK_CHANGE);
}
void Campaign_Auto_Repair_Walls()
{
    if(!GameActive||ScenarioInit||Frame%TICKS_PER_SECOND||GameToPlay!=GAME_NORMAL||!PlayerPtr||!PlayerPtr->IsHuman)return;
    for(int c=0;c<MAP_CELL_TOTAL;++c){
        auto& cell=Map[c];
        if(!Map.In_Radar(c)||cell.Owner!=PlayerPtr->Class->House||cell.Overlay==OVERLAY_NONE
            ||cell.OverlayData<16||!OverlayTypeClass::As_Reference(cell.Overlay).IsWall)continue;
        // A wall stores connection bits below bit four, then discrete damage levels.
        // Never recreate destroyed walls or change their connection pattern.
        int cost=MAX(1,int(OverlayTypeClass::As_Reference(cell.Overlay).DamagePoints)/4);
        if(PlayerPtr->Available_Money()<cost)continue;
        PlayerPtr->Spend_Money(cost);cell.OverlayData-=16;cell.Redraw_Objects();
    }
}
