// Local single-player conveniences; never apply to AI or multiplayer houses.
#ifndef CAMPAIGNPERKS_H
#define CAMPAIGNPERKS_H
inline bool Campaign_Local_Player(HouseClass const* house)
{
    return GameToPlay == GAME_NORMAL && house && house == PlayerPtr && house->IsHuman;
}
inline void Apply_Campaign_Unlimited_Power(HouseClass* house)
{
    // The reserve exceeds the maximum drain of all BUILDING_MAX structures.
    if (Campaign_Local_Player(house)) house->Power = MAX(house->Power, 1000000);
}
#endif
