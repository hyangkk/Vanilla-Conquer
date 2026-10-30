#ifndef TD_CAMPAIGNENEMIES_H
#define TD_CAMPAIGNENEMIES_H
class TechnoClass;
class FileClass;
class HouseClass;
class TechnoTypeClass;
bool Campaign_Enemy_House(HouseClass const* house);
void Campaign_Enemies_Produced(TechnoClass* object);
void Campaign_Enemy_Harvest(HouseClass* house,unsigned credits);
TechnoTypeClass const* Campaign_Enemy_Suggest(HouseClass const* house,RTTIType kind);
void Campaign_Enemies_Reset();
void Campaign_Enemies_Forget(const TechnoClass* object);
void Campaign_Enemies_Apply();
void Campaign_Enemies_Mark_Extra(const TechnoClass* object);
bool Campaign_Enemies_Save(FileClass& file);
bool Campaign_Enemies_Load(FileClass& file);
#endif
