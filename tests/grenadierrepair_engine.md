# Grenadier waves and player automatic repair

This diagnostic replaces the mission-start entry point only in a private probe
build, with dummy SDL drivers and a separate user/save directory. Restore
`missionstarts.cpp` before building or installing the release executable.

- A player `INFANTRY_E2` toggles optional reinforcements only when the paid product
  actually leaves its factory and `Factory.Completed` succeeds. Queued, cancelled,
  enemy and skirmish infantry never toggle it. Rifleman cash and grenadier power
  rewards remain. Repeated placement without a product never toggles again.
- Off is the default for new games and old saves. On immediately starts the former
  growing reinforcement preset: six troops initially, three more per wave,
  intervals gradually falling from 90 to 30 game seconds; at most twelve spawn
  attempts per one-second service pass. Legal reachable land and heap limits apply.
  Infantry/armor mix and dinosaur missions retain the former wave behavior.
- Off cancels the undelivered quota, preserving already arrived enemies, normal
  production and original mission scripts. Re-enabling continues wave progression.
  `CNCWAV02` stores the toggle, counters and pending delivery. `CNCWAV01` is read but
  intentionally starts off. Tests cover partial delivery across save/load.
- Player bike effective maximum/current HP doubles. Native damage, health ratio,
  repair cap and scenario INI ratios use the effective maximum. `CNCBIK02` stores a
  per-slot migration marker after existing ion cooldown records; `CNCBIK01`/EOF
  migrate damaged bikes proportionally, once. Ownership changes preserve the ratio.
- Player buildings automatically start original paid repair, including obelisks and
  advanced towers. Existing SAM fivefold maximum and original repair remain.
  Construction, deconstruction and exploding buildings do not start auto repair.
- All player mobile objects, including infantry, vehicles and aircraft, heal two
  percent of effective maximum per game second, at least one HP. Cost is proportional
  to HP restored, using one quarter of raw unit cost per original health bar, at
  least one credit per step. Full/dead/limbo objects do not heal or charge. Empty
  wallets wait and resume when funds return. Skirmish and enemy forces remain native.
- Owned concrete and other walls heal one discrete damage stage per game second,
  using funds and preserving connection bits. Destroyed walls are never recreated.

Negative controls remove the factory toggle, mobile/wall repair, player building
repair and doubled bike cap; the diagnostic must reject those behaviors. Run it
again after restoration, along with existing SAM, economy/bike, player production,
81-save compatibility and CTest regressions.

No GUI launch or screenshots are authorized for this task; this validates the engine
and isolated save data, not visual appearance or long-session frame rate.
