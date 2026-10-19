# Native sidebar and tower sight integration probe

`sidebargrid_engine.cpp` is a diagnostic replacement for `tiberiandawn/missionstarts.cpp`, using the offline mission-export hook. Never link it into a release or install its executable.

Prepare an isolated directory with `CONQUER.INI` pointing at installed assets and a private UserPath. Set `[Video] GameWidth=960` and `GameHeight=600`. Copy the 56 existing mission starts into `starts/savegame.100` through `.155`, and a GDI base save into `progress.sav`. Never use original saves as output targets.

Temporarily replace the exporter source with the probe, build, copy the executable into that directory, then restore the original exporter and rebuild the release. Run the probe with SDL_VIDEODRIVER=dummy, SDL_AUDIODRIVER=dummy, ALSOFT_DRIVERS=null and VANILLA_MISSION_START_DIR pointing at the isolated directory.

The probe checks all 56 legacy starts, a 240-pixel sidebar on the unchanged 960×600 canvas, 48×36 cameos, two columns per category with height-adaptive rows, all visible selection rectangles and matching production events, row scrolling to both ends, normalized legacy scroll state, clipping, tactical dimensions, a full-height power gauge, and a save/reload round trip. Scroll buttons use the actual SHP image height and are anchored to the bottom of the canvas; the full grid must end above them. It adds production facilities to the copied base and writes `savegame.090` for visual QA. Options.Set initializes the preview palette, and victory/loss flags are cleared after one-off house AI recalculation. These fixture changes are not production behavior.

Tower checks compare actual revealed cell sets against radius 10 for the player's Obelisk and radius 8 for the player's Advanced Guard Tower. Enemy towers retain their original sight, and loading existing towers reapplies the bonus without changing serialized object sizes. Only static selection-button capacity grows; per-save fields remain unchanged.

The macOS arm64 engine run reports zero failures, 22 building entries and 24 unit entries. CTest passes 15/15. Separate native visual QA must also inspect the bottom arrow buttons and crisp icons under hover; arithmetic checks alone do not verify the artwork height.
