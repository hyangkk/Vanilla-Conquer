# Native scenario opening credits regression

Use originalcredits_engine.cpp (campaigncredits_engine.cpp is a compatibility entry point) in the existing isolated mission-start probe harness. Supply starts.tsv with slot/root pairs for all 81 base, dinosaur and expansion starts, original/savegame.NNN copies, and a progress.sav with Frame > 0. Never run against live saves as output.

Require every fresh mission to retain its INI house Credits times 100. Load each pre-existing 50,000-credit frame-zero save and verify the original balance, InitialCredits, and both visible-credit counters. Save and reload again to verify idempotence and unchanged army counts. Compare a native carry-over opening with its save/reload, and ensure a progressed game's 1234 credits plus 56 stored Tiberium and InitialCredits=999 survive untouched. The restoration helper must itself ignore Frame > 0.

The old 50,000-credit regression and reports describe a superseded setting. Historical evidence is not a current expectation. Restore missionstarts.cpp after probing, rebuild the normal executable, then run CTest before installation.
