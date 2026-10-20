# Native SAM damage and radial blast probe

Use a private missionstarts.cpp substitution, SDL dummy video/audio, installed assets, private UserPath, and Blackout fixture.sav. Restore missionstarts.cpp immediately after building the diagnostic and rebuild the normal application before installing. Never modify live saves.

A real local SAM missile has 100 raw damage versus the unchanged enemy 50 with equal firepower bias. Native tank HP loss at the impact center must be exactly 100 versus 50. GDI/Nod ownership, AI and skirmish are checked.

Ground bursts use a fivefold radial distance and the existing AP armor/falloff curve evaluated at distance/5. The original nominal radius is 1.5 cells; the expanded radius is 7.5 cells, with a strict outside boundary. A dense fixture has over 32 targets and independently checks every target HP and cleared damage markers; a dynamic target list must not truncate the larger blast. Terrain and walls use the same radial falloff. Flying-aircraft blasts remain separate from ground victims and extend the same radial rule to nearby airborne aircraft.

The native BulletClass flight must damage both its aimed target and a secondary target four cells away for ground and airborne impacts. In-flight missiles survive native save/reload without multiplying damage again. Already-fired missiles retain their saved strength; new shots from existing saved SAMs acquire the perk through the normal firing path. Blast ownership is evaluated from the surviving firing SAM at impact, consistent with existing ownership-based perks.

Disable the two feature branches as a negative control, then restore and require zero failures. Run existing SAM range/sight/attack-speed and production/bonus probes plus CTest before rebuilding and installing the normal binary. No rendered UI or explosion-animation asset changes are part of this damage calculation.
