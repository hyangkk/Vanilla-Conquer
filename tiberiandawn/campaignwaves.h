#ifndef TD_CAMPAIGNWAVES_H
#define TD_CAMPAIGNWAVES_H
class FileClass;
void Campaign_Waves_Reset();
void Campaign_Waves_AI();
void Campaign_Waves_Toggle();
bool Campaign_Waves_Enabled();
bool Campaign_Waves_Save(FileClass& file);
bool Campaign_Waves_Load(FileClass& file);
#endif
