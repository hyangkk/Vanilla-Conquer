# Campaign fortress and expansion engine verification

`campaignexpansion_engine.cpp` is an isolated engine diagnostic, substituted for
`missionstarts.cpp` only in a temporary build and always restored before release.
Use SDL video/audio dummy drivers and a private user/save directory. Do not launch
or modify the live game or its saves. It includes shared SAM diagnostic helpers.

Coverage:

- Ten-cell inclusive construction radius, measured from an owned building's occupied
  footprint, not a wall/ally anchor; eleven cells and skirmish are excluded.
- Real Factory.Set/Start/AI/Completed transactions build both factions' paid bases:
  refineries, infantry/vehicle facilities, basic/heavy defenses, concrete walls.
- A yard-only enemy is activated and native Building.AI continuously buys and places
  a refinery and power without requiring fixed mission blueprint nodes.
- Clear terrain and footprint collisions remain mandatory. Facility spacing and
  cardinal wall gaps reserve egress. Producer-local placement encourages forward bases.
- Vehicles exit GDI factories and Nod airstrips within twelve cells when doors are
  obstructed. A fully blocked area retains the paid object for retry. Sidebar retry
  preserves identity and money rather than treating a wait result as completion.
- Nod airstrip output keeps its actual factory product identity, preserving the
  previously implemented exactly-three-for-one-cost quota.
- Newly released enemy combat units immediately receive squad gathering orders;
  harvesters harvest, MCVs receive separately chosen expansion orders.
- Wealthy AI buys MCVs as its target yard count rises every five game minutes to
  eight (tripled MCV output can overshoot that target). Reachable ground is flood
  searched; the vehicle actually drives and deploys into a new yard. Native NavCom
  preserves its route across save/load. Scripted team/trigger MCVs are preserved.
- Twenty native resource growth/spread steps per campaign tick exactly match twenty
  ordinary steps using the same saved map state and explicitly reset simulation RNG.
  Existing growth/spread options remain effective; skirmish uses one native step.
- No game object layouts change. Optional versioned formation state follows existing
  save footers; legacy EOF remains accepted. Existing saves use the new logic on load.

Regression controls temporarily restore a two-cell public proximity test, ordinary
resource rate and the sidebar's old boolean success test. The diagnostic must fail
those removed behaviors; sources are restored afterwards. Re-run the final diagnostic,
economy/production/bike suite, SAM suite, 81 legacy mission saves and player production,
then build the normal entry point and run CTest before installation.

Engine verification does not certify visual behavior or long-session frame rate.
The user's installation-only preference excludes UI launch, capture and restart.
