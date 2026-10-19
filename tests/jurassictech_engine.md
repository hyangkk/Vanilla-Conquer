# Jurassic campaign tech cold-load regression

Use the opt-in isolated exporter workflow from MISSION-START-SAVES.md, substituting
jurassictech_engine.cpp for missionstarts.cpp in a separate test build. Provide
original Jurassic mission starts as savegame.151 through .155, and normal campaign
starts as control.100 (GDI) and control.150 (Nod). Set CampaignAllTech=yes.
Never install the probe executable; restore missionstarts.cpp for release builds.

Before each Jurassic load the probe deliberately resets ScenPlayer to GDI and
clears Special. Require restored SCJ roots/Jurassic mode and unlocked eligibility
for all five starts, plus normal GDI/Nod mode when subsequently loaded. The test
checks disabling the option and AI scope. For mission four it deploys the actual
saved MCV on a clear legal location and finds a construction factory for both an
Obelisk and an advanced guard tower. No live save is changed or extra MCV granted.
