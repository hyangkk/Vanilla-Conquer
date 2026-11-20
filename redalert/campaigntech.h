// Custom build (Red Alert): campaign production rules for the local human player.
#ifndef RA_CAMPAIGNTECH_H
#define RA_CAMPAIGNTECH_H

class HouseClass;
class TechnoClass;
class TechnoTypeClass;
class ObjectTypeClass;

/*
**	With [Options] CampaignAllTech on (default), the local human campaign player may build every
**	normally buildable Allied/Soviet structure, infantry, vehicle, ship and aircraft, ignoring
**	faction, tech level and prerequisites, and even without a construction yard or factory.
*/
bool Campaign_All_Tech(const HouseClass* house);
bool Campaign_All_Tech_Type(const ObjectTypeClass* type, const HouseClass* house);
bool Ra4x_No_Factory(const ObjectTypeClass* type, const HouseClass* house);
bool Ra4x_Exit_Without_Factory(HouseClass* house, TechnoClass* object);
CELL Ra4x_Production_Anchor(const HouseClass* house);
void Ra4x_Add_All_Buildables(const HouseClass* house);

/*
**	Auto-targeting priority for human-owned objects: 3 = enemy attacking us, 2 = other enemy,
**	1 = unarmed enemy structure or wall (last resort), 0 = none.
*/
int Ra4x_Target_Rank(TechnoClass const* self, TARGET target);

#endif
