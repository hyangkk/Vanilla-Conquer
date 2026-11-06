Native HUD scroll regression
============================

Compile hudscroll_engine.cpp in place of the missionstarts.cpp export entry point,
then run vanillatd with VANILLA_MISSION_START_DIR pointing to a private directory.
Provide a copy of an existing campaign save as user-current.sav and the real game
assets through CONQUER.INI. Never overwrite the user's original save.

The harness loads the native save and executes 64 Map.Scroll_Map calls in eight
directions. Compare identical scroll sequences with and without the counter,
with smooth motion off and on. All world pixels outside the fixed HUD rectangle
must match exactly. Check every HUD pixel at interpolation fractions 0, .5, 1
on each frame, including an actual moving native tank. Also verify drawing and
restoring the overlay preserves every underlying world pixel. PCX and native
compositor PPM captures are written to the private output directory.

Negative control: omit Restore_Counts at the start of TabClass::Draw_It. The
outside-HUD comparison must fail. Restore the hook and rerun before building the
normal application; the probe entry point must not be installed.
