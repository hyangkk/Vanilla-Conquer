#ifndef TD_CAMPAIGNEXPANSION_H
#define TD_CAMPAIGNEXPANSION_H
class HouseClass;
class TechnoClass;
class TechnoTypeClass;
class BuildingClass;
bool Campaign_Expansion_Near(HouseClass const* house, CELL cell);
TechnoTypeClass const* Campaign_Expansion_Suggest(HouseClass const* house, RTTIType kind);
int Campaign_Expansion_Exit(BuildingClass* producer, TechnoClass* product);
void Campaign_Expansion_AI();
void Campaign_Expansion_Resources();
#endif
