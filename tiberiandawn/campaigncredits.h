// Campaign opening balance; keep ordinary in-progress saves untouched.
#ifndef CAMPAIGNCREDITS_H
#define CAMPAIGNCREDITS_H
inline void Apply_Campaign_Starting_Credits()
{
    if (GameToPlay == GAME_NORMAL && PlayerPtr && PlayerPtr->IsHuman) {
        PlayerPtr->Tiberium = 0;
        PlayerPtr->Credits = 50000;
        PlayerPtr->InitialCredits = 50000;
        PlayerPtr->VisibleCredits.Credits = 50000;
        PlayerPtr->VisibleCredits.Current = 50000;
        PlayerPtr->VisibleCredits.IsToRedraw = true;
    }
}
#endif
