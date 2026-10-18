# Native Obelisk ownership verification

`obeliskrange_engine.cpp` is an opt-in integration probe using the actual game
objects, Weapon_Range, In_Range, Can_Fire and Fire_At implementations. It is not
compiled into the installed game and requires original game assets.

In an isolated source/build checkout, substitute this file for
`tiberiandawn/missionstarts.cpp` and build VanillaTD. The existing offline exporter
entry point invokes it after normal game initialization. Place the executable
and CONQUER.INI in a separate verification directory. Set Paths.DataPath to the
installed assets, Paths.UserPath to the verification directory, Intro.PlayIntro=no,
and Video.Windowed=yes. Copy a GDI mission-start save there as `fixture.sav`.
Run the executable with SDL_VIDEODRIVER=dummy, SDL_AUDIODRIVER=dummy,
ALSOFT_DRIVERS=null and VANILLA_MISSION_START_DIR set to that directory.
Exit zero and TOTAL FAILURES 0 are required. Never install this probe executable.

The original all-owner change (3668f3b) fails nine ownership/range checks. The
player-only correction passes all 25 checks, including the real laser projectile
creation at 12 cells, enemy FIRE_RANGE at 12 cells and enemy FIRE_OK at 7 cells.
Coordinates are tested at 7, 8, 12, 15 cells and one lepton past 15 cells. The probe
also covers GDI and Nod player identity, owner pointer changes, AI control,
skirmish exclusion, unchanged guard tower range, laser damage and rate of fire.
The owner pointer checks exercise the runtime scope, not the engineer capture UI.
