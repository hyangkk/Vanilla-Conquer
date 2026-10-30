#ifndef TD_CAMPAIGNTRAFFIC_H
#define TD_CAMPAIGNTRAFFIC_H
class FootClass;
void Campaign_Traffic_AI();
void Campaign_Traffic_Reset();
void Campaign_Traffic_Forget(TARGET id);
void Campaign_Traffic_Order(FootClass* object,TARGET destination);
bool Campaign_Traffic_Wait(FootClass* object);
#endif
