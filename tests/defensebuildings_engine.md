# Campaign defense building targeting integration probe

In an isolated source/build checkout substitute defensebuildings_engine.cpp for
missionstarts.cpp, then build VanillaTD using the offline exporter setup in
MISSION-START-SAVES.md. Provide fixture.sav (GDI mission-start save) and a separate
CONQUER.INI whose DataPath points to original game assets and UserPath points to
the isolated output directory. Use dummy SDL video/audio and null OpenAL drivers,
and set VANILLA_MISSION_START_DIR to that output directory. Never install the
probe executable; restore missionstarts.cpp before release building.

The probe uses real building placement on clear cells, enemy power plants ten
cells away, Greatest_Threat, Mission_Guard, Can_Fire, Mission_Attack and projectile AI.
It checks target acquisition, attack assignment and actual structure damage for
both advanced guard towers and Obelisks. It also checks visibility, friendly and
out-of-range exclusions, ordinary guard tower and skirmish exclusions, Nod player
ownership and unchanged enemy AI eligibility. Require exit zero and no FAIL lines.
The native attack mission fires after autonomous guard acquisition; this is not
a timed complete battle or destruction replay.
