# Squad staging, formation movement and construction routes

Native isolated engine diagnostic using the same dummy SDL/private user directory
workflow as `grenadierrepair_engine.cpp`. Never install its substituted entry point.

Campaign enemy combatants from real production, paid companions and optional waves
join local squads of at most twelve. Existing armies migrate at most twenty-four
members per game second. Harvesters, MCVs, transports, loaners, native teams,
triggers and ambush commands are preserved. Player/skirmish orders are untouched.

Squads gather on distinct reachable open-ground slots. At least eight members with
four in position can advance early; otherwise gathering expires after twenty game
seconds. Armor leads, infantry occupies rear rows, and approach destinations are
spread perpendicular to the attack direction. Approach expires after twenty-five
seconds. Nearby threats launch an immediate attack. These bounds prevent indefinite
waiting for dead, blocked or missing members.

The optional `CNCFRM01` footer follows bike state without changing game object
layouts. It stores squad membership, phase, unique slots and remaining deadlines;
legacy EOF starts with no groups. Native Suspended fields belong exclusively to
traffic. Companion synchronization skips temporary overrides and formation holding.
A member finishing traffic yield rejoins an already launched group. Mixed groups
with casualties use a strict armor/nonarmor ordering. Nearby disconnected land
regions cannot share an unreachable rally; each approach uses that member's own
reachable terrain component.

Building candidates are scored first, then checked locally before placement. A
candidate cannot disconnect the previously open neighboring travel cells around
its foundation. Factories and concrete walls retain spacing/gates, and MCV site
selection/deployment applies the same route preservation rule. Checks consider
static terrain/buildings and ignore temporary mobile occupancy. The bounded local
check can conservatively reject a site whose only alternative route is farther
away. It applies to automatic enemy construction, preserving player placement freedom.

Tests cover distinct gathering/approach slots, actual native group travel, timeout
release, saved membership, three-for-one production without slot overwrite, traffic
handoff at launch, casualties, disconnected terrain, emergency defense, exclusions,
one-cell alley closure versus an adjacent surviving route, and actual native enemy MCV
deployment refusal in a construction-width corridor. Controls disable
recruitment and route protection and must fail those scenarios.

Campaign resource processing now performs twenty unchanged native steps per tick:
five times the former four-step setting. Skirmish remains one step, and existing
growth/spread options remain effective. Fixed map/RNG comparisons verify 150 wrapper
calls equal 3000 ordinary calls. This changes regrowth, not instant full-map coverage.

No GUI or long-session performance claim is made by these engine diagnostics.
