#ifndef TD_CAMPAIGNBIKE_H
#define TD_CAMPAIGNBIKE_H
class TechnoClass;
class UnitClass;
class FileClass;
bool Campaign_Bike_Active(TechnoClass const* object);
void Campaign_Bike_AI(UnitClass* bike);
void Campaign_Bike_Forget(TARGET id);
void Campaign_Bike_Reset();
bool Campaign_Bike_Save(FileClass& file);
bool Campaign_Bike_Load(FileClass& file);
#endif
