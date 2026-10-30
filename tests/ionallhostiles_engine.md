# Broad hostile ion targets and real-loop automatic-repair verification

Use a private probe build with `ionallhostiles_engine.cpp` substituted only for
`missionstarts.cpp`, dummy SDL drivers and a separate user/save directory. Restore
the normal entry point before building the app. Do not access the running game UI.

The running application process must be distinguished from the installed file.
During this task, the user's process began at 15:04 while the automatic-repair
binary was installed at 16:54 on October 30, 2026. Replacing the bundle's file does
not update the already running process. No automatic-repair implementation change
was justified: full `Logic.AI`, rather than a helper-only test, confirmed native
repair of player infantry, vehicles, aircraft, obelisk, advanced tower and ordinary
buildings within three game seconds. Existing money/cap/dead/limbo rules remain.
Do not restart the live game without the user's approval; their installation-only
preference remains effective until changed.

The bike ion pulse keeps its existing one-game-second cooldown, ten-cell range,
random choice, original ground animation and separate airborne-hit handling.
Enemy-owned civilian/objective objects participate regardless of normal weapon-AI
`IsLegalTarget` filtering. For buildings on a visible cell, an unset legacy
`IsDiscoveredByPlayer` flag no longer prevents targeting. Ordinary hidden/cloaked
unit rules remain; player, allied and neutral objects are excluded.

Walls are owned cell overlays, so they must be collected separately from Techno
heaps. Only visible hostile wall cells inside the actual ten-cell radius qualify.
The native ion animation, including native wall damage stages, resolves the hit;
no new instant wall destruction or damage multiplier is introduced. Wall collection
scans a bounded local square, not the whole map on every bike pulse.

Tests cover multiple enemy civilian buildings in discovered and visible-but-not-
discovered states, concrete/sandbag/cyclone/barbed-wire/wood walls, real damage from
the native animation, hostile civilian infantry, ownership/visibility/range
exclusions, and the real automatic-repair simulation loop. Restoring the former
bike source must fail the new target tests. Run the existing economy/bike and
repair/wave save regressions, then normal build and CTest before installation.

Engine verification does not certify the screen or change an old live process.
