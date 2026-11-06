Native unit regeneration regression
===================================
Use allunithealing_engine.cpp as the isolated mission-start probe entry point.
Provide private fixture.sav and user-current.sav copies and real installed assets.
Do not write any output into the user's live save directory.

Reproduce a zero-credit damaged rifleman through 45 actual Logic.AI ticks. The
old paid repair must fail; free mobile regeneration must increase HP. Run the
same loop for all 20 infantry variants, tank, bike, MCV, harvester, both siege
forms, Orca and transport helicopter. Assert no money is spent/generated, HP is
bounded and saved health survives reload. Exclude enemies, skirmish and dead
objects. Produce an actual native PCX of the tested army.

Building and wall repair retains its previous paid rules. Mobile healing runs
once per 15 simulation ticks at floor(max HP / 50), with a minimum of 1 HP.
Objects in limbo or destroyed are not resurrected.
