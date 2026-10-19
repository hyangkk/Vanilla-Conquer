# Native player SAM ground combat probe

Temporarily replace `tiberiandawn/missionstarts.cpp` with `samground_engine.cpp`, build and copy the diagnostic binary, then restore the exporter source and rebuild the normal release. Never install a diagnostic binary.

Prepare a private `CONQUER.INI` pointing at installed assets and a private UserPath with a copy of a large GDI scenario as `fixture.sav` (the verified COV Blackout opening save works). Run the diagnostic using the normal offline hook `VANILLA_MISSION_START_DIR`, SDL dummy video/audio and null OpenAL.

The probe clears existing objects into limbo in the private fixture and places a player SAM and a target 20 cells apart. It independently checks a vehicle, infantry, an unarmed building and an airborne Orca. It exercises automatic scanning, manual attack eligibility, idle guard assignment, and the actual SAM rise/turn/fire animation through `BuildingClass::AI` before ticking real missile objects. Vehicle, structure and air targets must take damage. Infantry must be acquired and fired upon; the original AP missile scatter and warhead accuracy remain intact.

It checks player range equals the original weapon range ×3 (22.5 cells), out-of-range rejection, unchanged 50 damage / 50 ROF, enemy ground exclusion and existing range, AI and skirmish exclusion, both player factions and save/reload of an existing SAM. The previous campaign-wide Nike ×2 branch is preserved for other owners; this change adds only the local SAM ×3 override. The weapon and bullet class data, combat cadence and serialized object layouts remain unchanged.

The fixture disables its copied mission trigger actions and fixes its starting camera so the combat preview cannot end due to unrelated campaign objectives. These are diagnostic-only changes.

The fixture writes `savegame.095` only in its private directory for separate visual QA. The macOS arm64 native run completed with zero failures; CTest passed 15/15. Ground eligibility must be allowed both in the scan/action layer and in all four SAM ready/firing states; enabling only the scan makes the launcher discard a ground target before firing.
