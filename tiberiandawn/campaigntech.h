// Optional single-player campaign production rules; no saved object layout changes.
#ifndef CAMPAIGNTECH_H
#define CAMPAIGNTECH_H
class HouseClass;
class TechnoTypeClass;
namespace CampaignTech {
inline bool Enabled(bool option, bool campaign, bool local_player, bool human, bool faction) {
    return option && campaign && local_player && human && faction;
}
inline bool Eligible(bool enabled, bool buildable, bool faction_type, int level, int scenario) {
    return enabled && buildable && faction_type && level < 99 && scenario < 99;
}
}
bool Campaign_All_Tech(const HouseClass* house);
bool Campaign_All_Tech_Type(const TechnoTypeClass* type, const HouseClass* house);
#endif
