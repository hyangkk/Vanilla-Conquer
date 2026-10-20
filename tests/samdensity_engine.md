# SAM density volleys and twofold damage

Use the existing private diagnostic substitution workflow with Blackout fixture.sav, dummy SDL, and isolated UserPath. Restore missionstarts.cpp immediately after building and rebuild the normal binary before installation.

Count hostile active mobile units (vehicle, infantry, aircraft) in the closed ten-cell circle measured from SAM center. Exclude buildings, own/allied/neutral actors, dead/limbo actors and actors outside the radar map. Each ordinary missile adds floor((enemy count + fractional fifths)/5) real missiles at its normal target. Retain remainder modulo five for that SAM. Base-class firing bypasses recursive volleys, and the base salvo rearm/second-shot state must remain unchanged. If the native bullet heap is full, the engine cap limits physical missiles.

Zero, one, four, five, ten and twenty hostiles must produce 5,6,9,10,15,25 missiles across five ordinary shots. Mixed vehicle/infantry/aircraft and exactly ten cells versus ten cells plus one lepton are checked. A save after one fifth must restore the remainder; a destroyed/recycled building slot must start with no remainder. Use Campaign_Enemies_Mark_Extra for synthetic final-population fixtures so the separate enemy multiplier does not alter population on load.

Append optional CNCSAM01 fractional bytes after the existing wave footer without altering native object layouts. Every byte must be 0..4. Old saves omit this extension and start with zero remainder.

The included blast regression checks current player damage 100 versus unchanged enemy50, actual ground/air missile flight and collateral damage, unchanged7.5-cell radius, and saved missile strength. Existing saved missiles keep their stored power; new shots from existing SAMs use2x power. Engine simulation tests do not claim rendered UI verification.
