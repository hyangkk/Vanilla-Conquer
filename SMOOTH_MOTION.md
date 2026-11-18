# Display-only smooth motion (Tiberian Dawn / SDL2)

This branch is based on the installed r973 / ce83b59 game. Simulation timing, unit
statistics, AI, navigation, targeting, RNG and gameplay coordinates are unchanged.
The renderer samples a short history of completed simulation positions at a delayed
presentation timestamp (100 ms at GameSpeed=4; 1.5 ticks for slower settings).
Pending simulation snapshots never replace published positions or timestamps during
the input redraw before Logic.AI. Actual sample timestamps absorb uneven tick
intervals without extrapolating through blockers. Commands still enter the original
input/command path immediately.

Enable with `[Video] SmoothMotion=yes` (default); `SmoothMotion=no` restores the
original raster presentation. Set `FrameLimit=120` for a 120 Hz display. A display
cap alone does not create intermediate movement positions.

The original 8-bit rasterizers capture coverage into separate ordered layers,
including pixels that have the same colour as their background. Scene capture
runs at the existing game/input redraw cadence. At each display presentation,
the output-resolution compositor interpolates positions and blends subpixel edge
coverage while preserving sharp pixel-art interiors. SDL2/Metal presents that frame.
Palette-table shadows and translucent fog are evaluated against the newly composed
background, so terrain is never baked into a moving shadow. Static occluders, fog
and the foreground remain in draw order. Menus/movie blits invalidate the captured
scene. Coverage state is external to saved objects; scenario/load and object lifetime
hooks reset it. Large position jumps snap. Predator distortion/cloaking falls back
to the original raster for that scene to preserve the effect safely.

Ground motion for infantry, vehicles, aircraft and bullets is interpolated. Sprite
animation poses and aircraft altitude retain the original logic cadence. Multiplayer
and every campaign/visual effect have not been exhaustively playtested.

## Build and checks

```sh
cmake -S . -B build/smooth -G Ninja -DBUILD_VANILLARA=OFF -DBUILD_TESTS=ON -DMAKE_BUNDLE=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build/smooth
ctest --test-dir build/smooth --output-on-failure
```

The interpolation test covers endpoint continuity, reverse movement, pause/stall
clamping, teleports and immutable inputs. The raster-layer test checks exact
reconstruction, identical-colour coverage, solid fog, primitive coverage, immutable
published frames and menu invalidation. Original raster tests also run unchanged.

For a bounded local presentation trace, set `VANILLA_MOTION_TRACE` to a writable CSV
path before launch. It records at most 1800 selected-object presentation samples with stable world
anchors and a pre-logic redraw phase flag. This
measures application draw calls, not the physical display's scanout. It is disabled
by default and does not alter gameplay inputs.

The local custom app is packaged separately from the original and uses the existing
data/save directory. No copyrighted game assets, binaries, saves, traces or local
paths are included in this Git branch.
