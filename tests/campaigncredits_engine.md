# Campaign opening credits integration probe

In an isolated build, substitute campaigncredits_engine.cpp for missionstarts.cpp.
Use the existing offline exporter setup described in MISSION-START-SAVES.md.
In the isolated user directory provide missions.txt, original/savegame.100 through
original/savegame.155 (the original frame-zero mission starts), and in-progress.sav
(a normal save at Frame > 0). Never use the live user directory as output.

The probe loads all original starts and checks total money and displayed money are
50000, round-trips an in-progress save with 1290 total money and checks preservation,
then starts all listed missions and verifies fresh balances and cold save/load.
Require exit zero, 56 original-start confirmations, the progress confirmation, and
56 verified.tsv rows with credits=50000 and frame=0. Generated files are test output.
Do not install the probe binary. Restore missionstarts.cpp before release building.
