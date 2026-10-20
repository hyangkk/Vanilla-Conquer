# SAM density volleys and stock damage

Use the existing private diagnostic substitution workflow with Blackout fixture.sav, dummy SDL, and isolated UserPath. Restore missionstarts.cpp immediately after building and rebuild the normal binary before installation.

Count hostile active mobile units (vehicle, infantry, aircraft) in the closed circle of half the current weapon range (currently22.5cells) measured from SAM center. Exclude buildings, own/allied/neutral actors, dead/limbo actors and actors outside the radar map. Each ordinary missile adds floor(enemy count/2 + saved fraction) real missiles at distinct legal secondary targets, excluding the primary target. Retain fraction in tenths for that SAM. Base-class firing bypasses recursive volleys, and the base salvo rearm/second-shot state must remain unchanged. If the native bullet heap is full, the engine cap limits physical missiles.

Zero, one, four, five, ten and twenty hostiles must produce 2,3,6,7,12,22 missiles across two ordinary shots. Mixed vehicle/infantry/aircraft and exactly half-range versus half-range plus one lepton are checked. A save after one half must restore the remainder; a destroyed/recycled building slot must start with no remainder. Use Campaign_Enemies_Mark_Extra for synthetic final-population fixtures so the separate enemy multiplier does not alter population on load.

Append optional CNCSAM03 fractional bytes and per-slot health-boost markers after the existing wave footer without altering native object layouts. Every fractional byte must be0..9 and each health marker0..1. Legacy CNCSAM01 bytes0..4 are converted from fifths to tenths without loss; CNCSAM02 retains its tenths, and neither version contains a health marker. Invalid data is rejected. Old saves omit this extension and start with zero remainder.

The included blast regression checks current player damage25 is half unchanged enemy50, actual ground/air missile flight and collateral damage, unchanged7.5-cell radius, and saved missile strength. Existing saved missiles keep their stored power; new shots from existing SAMs use0.5x power. Engine simulation tests do not claim rendered UI verification.
