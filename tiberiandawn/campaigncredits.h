// Restore native opening balances in existing frame-zero campaign saves.
// Fresh missions use Read_Scenario_INI unchanged, including native carry-over.
#ifndef CAMPAIGNCREDITS_H
#define CAMPAIGNCREDITS_H
#include <cstdio>
inline void Restore_Campaign_Starting_Credits()
{
    if (GameToPlay != GAME_NORMAL || Frame != 0 || !PlayerPtr || !PlayerPtr->IsHuman) return;
    char filename[sizeof(Scen.ScenarioName) + 5];
    std::snprintf(filename, sizeof(filename), "%s.INI", Scen.ScenarioName);
    CCFileClass file(filename);
    CCINIClass ini;
    if (!file.Is_Available() || !ini.Load(file, false)) return;
    int credits = ini.Get_Int(PlayerPtr->Class->IniName, "Credits", 0) * 100;
    int percent = Cardinal_To_Fixed(100, ini.Get_Int("Basic", "CarryOverMoney", 100));
    int carryover = Fixed_To_Cardinal(Scen.CarryOverMoney, percent);
    int cap = ini.Get_Int("Basic", "CarryOverCap", -1);
    if (cap != -1) carryover = MIN(carryover, cap);
    credits += carryover;
    PlayerPtr->Tiberium = 0;
    PlayerPtr->Credits = credits;
    PlayerPtr->InitialCredits = credits;
    PlayerPtr->VisibleCredits.Credits = credits;
    PlayerPtr->VisibleCredits.Current = credits;
    PlayerPtr->VisibleCredits.IsToRedraw = true;
}
#endif
