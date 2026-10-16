# Campaign all-tech option

Set `CampaignAllTech=yes` in the `[Options]` section of `vanillatd.ini`.
The default is off. This local campaign option applies only to the human
GDI/Nod player in normal single-player games, including loaded saves.

All normally buildable GDI/Nod buildings, infantry, vehicles and aircraft
ignore faction ownership, mission number, tech level and prerequisite limits.
A matching production facility is still required: construction yard, barracks
or Hand of Nod, weapons factory or airstrip, and helipad. Both factions' products
can use either faction's factory of the same category. Costs, build times,
combat statistics, resources, enemy AI and mission triggers are unchanged.

Civilian structures, dinosaurs, cargo aircraft and other non-production story
objects are not added. Missions without production facilities do not receive
an automatic base or extra MCV. Superweapon charging and mission-specific
special-weapon rules remain original. Set the option to `no` and restart to
return to normal campaign rules; existing placed units/buildings remain.

The save format and sidebar storage capacity are unchanged. Loading an older
save requests a production-list refresh. The existing sidebar capacity check
uses a strict bound to avoid writing beyond its fixed array.
