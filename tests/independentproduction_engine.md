# Independent production and one-second defenses

Use the private missionstarts.cpp substitution workflow, restore sources immediately, and rebuild the normal executable before installation. Use Blackout fixture.sav, live assets via DataPath, and isolated UserPath. No production saves are written.

Remove all physical bases and MCVs from play; retain a single soldier. For GDI and Nod, all 46 normal faction-buildable types must stay in the sidebar after recalc, start production through HouseClass, charge the original cost, and leave production without a physical factory. Buildings enter manual placement with a null builder and place on legal cells. Walls must become native overlays, rather than remain BuildingClass instances. Mobile units must enter the real map.

The three player defenses must complete after exactly 15 game ticks with zero physical construction yards; save/reload at tick five must preserve both stages and deadline. No object layout or save extension changes are used. Existing other-building, enemy, and campaign-wave probes remain applicable.

Completed stationary defenses fire 34 real projectiles at persistent targets. Skip warm-up and compare 30 intervals: stock/enemy versus current player Obelisk 2790/1380, Advanced Guard Tower 780/390, SAM 1995/495. The previous player SAM profile was 1005 ticks, so the new speed is 2.0303x that profile. Integer simulation ticks account for the small difference from exact doubling. Compare against the previous implementation as a negative control; then restore and rerun.

SAM range, sight, ground/air targeting, and existing-save ownership retain their separate samupgrade_engine.cpp coverage. savegame.093 is a private, fully revealed visual fixture with one soldier, no construction yard, no MCV, and the populated sidebar; no fixture visibility or trigger changes ship.

Every eligible infantry type additionally checks a 100,000-credit bonus after native exit. Duplicate exit events and save/reload must retain the exact balance without another award. Production costs remain separately charged.
