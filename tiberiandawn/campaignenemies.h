#ifndef TD_CAMPAIGNENEMIES_H
#define TD_CAMPAIGNENEMIES_H
class TechnoClass;
class FileClass;
void Campaign_Enemies_Reset();
void Campaign_Enemies_Forget(const TechnoClass* object);
void Campaign_Enemies_Apply();
bool Campaign_Enemies_Save(FileClass& file);
bool Campaign_Enemies_Load(FileClass& file);
#endif
