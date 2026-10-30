#ifndef TD_CAMPAIGNFORMATION_H
#define TD_CAMPAIGNFORMATION_H
class FootClass;
class FileClass;
class BuildingTypeClass;
void Campaign_Formation_Recruit(FootClass* object,bool producedCompanion=false);
bool Campaign_Formation_Holding(FootClass const* object);
void Campaign_Formation_AI();
void Campaign_Formation_Reset();
void Campaign_Formation_Forget(TARGET id);
bool Campaign_Formation_Save(FileClass& file);
bool Campaign_Formation_Load(FileClass& file);
bool Campaign_Construction_Preserves_Paths(BuildingTypeClass const& type,CELL cell);
#endif
