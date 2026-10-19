# Native SAM sight probe

Temporarily substitute `samsight_engine.cpp` for `tiberiandawn/missionstarts.cpp`, build a private vanillatd, and restore the original source immediately. Run with SDL dummy video/audio, ALSOFT_DRIVERS=null, and VANILLA_MISSION_START_DIR pointing to a private folder containing fixture.sav (Covert Operations Blackout start), CONQUER.INI and live DataPath. Never install the probe executable.

Checks local GDI/Nod sight 3 -> 30, enemy/AI/skirmish unchanged, actual inside/outside shroud cells, exhaustive reference at map corners, incremental outer ring, and existing-save load fixup. Saves isolated savegame.096 for actual rendering. All original mission triggers/objects are disabled in this private fixture. Radius ten deliberately retains its legacy hand-authored offset shape; exact-disk comparisons apply to radii 3, 8, 15 and the new 30 path.

Restoring the old MapClass::Sight_From ten-cell cap must fail the thirty-cell tests. Restore the implementation, rerun, then rebuild the normal application before installation. No probe state or saves belong in the production user folder.
