# Expansion mission starts (C&C 1)

Source data: the C&C 1.06c r3 u6 distribution linked by CnCNet and CNC-Comm:
https://cnc-comm.com/command-and-conquer/downloads/the-game/installer

Extract `app/sc-001.mix`, `app/sc-n64.mix`, `app/sc-psx.mix`, `app/general.mix`, and `app/scores.mix` using innoextract. Do not run the Windows installer. Copyrighted game data and generated saves are local artifacts and must not be committed.

Run `scripts/prepare_expansion_starts.py EXTRACTED_APP NEW_OUTPUT_DIR INSTALLED_DATA`. It prepares 15 Covert Operations missions, four Nintendo 64 missions, five PlayStation Special Ops missions and PATSUX, plus the expansion disc search path. Existing five Jurassic missions are already provided separately. Remap console filenames to avoid Covert Operations collisions; PATSUX must use scenario 72 for the engine's laser Orcas. Use native WINTER instead of the patch-only SNOW theater and restore the N64 build level to 15.

The combined GENERAL.MIX and SCORES.MIX must both be in `covertops/`. Without SCORES.MIX, all 25 saves cold-load but interactive entry can hang in `ThemeClass::Next_Song`, which has no exit when every track is unavailable. A process sample reproduced that exact loop. The diagnostic below checks at least one allowed track for every mission: missing music produced 25 failures, adding the distribution's soundtrack produced zero failures, and native UI entry into SCG71EA then succeeded.

Build the normal engine and run it with `VANILLA_MISSION_START_DIR=OUTPUT_DIR` and `VANILLA_MISSION_START_SLOT=200`, dummy SDL video/audio and null OpenAL. The optional `descriptions.txt` supplies one nonempty label of at most 40 bytes per mission. Slots default to 100, must be numeric and fit in the three-digit namespace; existing saves and verified.tsv are preflighted before writing.

For additional verification, temporarily replace `tiberiandawn/missionstarts.cpp` with `tests/expansionstarts_engine.cpp`, build and copy the diagnostic executable, then restore the exporter source and rebuild the normal release. Never install a diagnostic executable. Run it against the prepared export directory in dummy-driver mode. It checks all 25 cold loads, Frame 0, scenario identity, 50,000 credits, all tech, unlimited power, soundtrack availability, ordinary versus PATSUX Orca weapons, and all terrain images in the N64 winter map.

The macOS arm64 run passed, including zero missing terrain cells. A second export to the same slot range correctly failed before modifying any of the 25 saves. Installation copied only new mission INI/BIN files, the two expansion MIX files and savegame.200 through .224; SHA-256 checks confirmed all 73 existing saves unchanged. Actual save-list and N64-entry screenshots were captured in a separate preview app. Restart the existing game to discover the new expansion disc directory; a running session caches its disc inventory.

Opening-state saves are verified; complete mission playthroughs and missing expansion FMV cutscenes are outside this verification.
