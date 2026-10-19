# Campaign construction and power integration probe

In an isolated source/build checkout substitute campaignperks_engine.cpp for
missionstarts.cpp and use the offline exporter setup from MISSION-START-SAVES.md.
Provide starts/savegame.100 through .155 (original frame-zero mission saves),
and progress.sav (a GDI save after frame zero, with a construction yard).
Keep all outputs isolated and never install the probe executable.

Require exit zero and no FAIL lines. The probe checks all 56 existing starts,
a normal in-progress load, fresh scenario initialization, power replenishment
on a simulation tick, and an unchanged enemy low-power house. It calculates
the maximum possible building drain and checks the reserve exceeds it.
It finds legal distant terrain with both proximity checks disabled in skirmish,
then verifies the local campaign cursor and placement checks permit it.
A completed FactoryClass object is placed through HouseClass::Place_Object and
verified in the real map, then saved and reloaded. Overlap, map boundaries,
enemy cursor scope and skirmish scope remain tested independently.
