# Local campaign construction and power

In normal single-player games, the local human player may place buildings on
legal terrain without adjacency to the existing base. Terrain, map boundaries,
occupied cells, construction costs and production facility requirements remain.
Both cursor validation and actual placement use the same scope.

New missions and all loaded saves receive a power reserve of at least 1,000,000.
The reserve is maintained each simulation tick and exceeds the maximum possible
drain of all structures allowed by the building heap. The existing Power field
is serialized normally; no save format changes are required. This replaces the
need to produce a grenadier to obtain enough power.

AI houses and multiplayer/skirmish games keep their original rules. Existing
progress saves retain their money; the separate 50,000 opening-balance policy
still applies only to fresh starts and frame-zero saves.
