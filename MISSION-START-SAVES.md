# Native mission start saves

The opt-in `VANILLA_MISSION_START_DIR` environment variable runs an offline export
mode after normal game initialization, before entering menus or any logic ticks.
It is not enabled in the installed app. No normal gameplay behavior is changed.

Prepare an isolated user/output directory with `missions.txt`: one existing root
per line, for example `SCG01EA`, `SCB13EC`, `SCJ05EA`. Accepted roots are SC + G/B/J
+ two digits (01–99) + E/W + A/B/C/D. Duplicate/malformed roots and existing output
files are rejected. Give the exporter a separate CONQUER.INI beside its executable:
set Paths.DataPath to installed game data and Paths.UserPath to the isolated output
directory, Intro.PlayIntro=no. Run with a dummy SDL video/audio backend for offline
export, or a normal supported backend. Original game data is required.

The exporter creates savegame.100 upward, checks every target before writing, loads
each original scenario with normal difficulty and zero carryover credits, then saves
at Frame 0 using Save_Game. It immediately reloads every file using Load_Game and
checks mission root, frame, house, credits and counts for vehicles, infantry, buildings
and aircraft. Successful entries go in verified.tsv. Failure exits nonzero; partial
output stays isolated and must not be installed as a complete collection.

Only verified files should be copied into the live user directory, with exclusive
creation and a backup/hash manifest of existing saves. No old save is overwritten.
Generated files/assets are not committed to this source repository.

The installed base-game data on the development Mac contains 56 single-player roots:
26 GDI variants, 25 Nod variants, 5 Jurassic bonus missions. Expansion missions require
their own installed data. Saves are original map starts, without prior-mission carried
money or campaign sabotage choices. They use the native save format and inherit the
installed app's campaign tech option on load. The existing app can discover newly
added saves by reopening Load Mission, without an executable rebuild/restart.
