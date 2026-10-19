# Native sidebar grid integration probe

`sidebargrid_engine.cpp` is a diagnostic replacement for `tiberiandawn/missionstarts.cpp`, using the existing offline mission-export hook. Do not link it into a release or install its executable.

Prepare an isolated directory with `CONQUER.INI` pointing at installed game assets and a private UserPath. Copy the 56 existing mission starts into `starts/savegame.100` through `.155`, and an existing GDI base save into `progress.sav`. Never use original saves as output targets.

Temporarily replace the exporter source with the probe, build, copy the executable into that directory, then restore the original exporter and rebuild the release. Run the probe with SDL_VIDEODRIVER=dummy, SDL_AUDIODRIVER=dummy, ALSOFT_DRIVERS=null and VANILLA_MISSION_START_DIR pointing at the isolated directory.

The probe checks all 56 old starts, sixteen visible entries per category, all 32 hit rectangles and matching production events, two-item row scrolling to both ends, normalization of old scroll state, sidebar clipping, tactical width, and a save/reload round trip. It adds revealed production facilities to the copied base and writes only `savegame.090` for separate visual QA. Options.Set initializes the preview palette and pending victory/loss flags are cleared after the one-off house AI recalculation. These fixture adjustments are not production changes.

The compact layout preserves the original 160-pixel sidebar, uses 32×24 cameos in a 640×400 game frame, and shows two columns × eight rows per category. Cameos, clocks, and ready/hold overlays are composed together at native resolution before scaling.

Native engine result on macOS arm64: zero failures, 22 buildings and 24 units in the production lists. Existing CTest suite: 15/15 passed. Visual QA uses a separate C&C Sidebar Preview app and private saves; it must not interrupt a user's running game.

The expanded-view regression run uses `[Video] GameWidth=960` and `GameHeight=600`.
It checks all 56 legacy starts, the right-anchored 160-pixel sidebar, 584-pixel
viewport height, all 32 selection events, scrolling, and a save/reload round trip.
The 2026-10-19 macOS arm64 dummy-driver run reported zero failures. Native Retina
preview additionally verified a production tooltip to the left of the entire
sidebar and crisp cameos under the software cursor in fullscreen.
