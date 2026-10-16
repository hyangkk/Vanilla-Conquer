// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
// software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.

// TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed
// in the hope that it will be useful, but with permitted additional restrictions
// under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT
// distributed with this program. You should have received a copy of the
// GNU General Public License along with permitted additional restrictions
// with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection
// Indexed raster layers retain the original art; only presentation positions are fractional.
#ifndef RENDERLAYERS_H
#define RENDERLAYERS_H
#include <chrono>
#include <cstdint>
#include <vector>
#include <array>
class GraphicViewPortClass;
namespace RenderLayers {
struct Layer {
    int x, y, width, height;
    double start_x, start_y, end_x, end_y;
    std::vector<unsigned char> pixels, mask;
    std::vector<unsigned short> effects;
};
struct Frame {
    int width = 0, height = 0, clip_x = 0, clip_y = 0, clip_w = 0, clip_h = 0;
    bool ready = false, publishing = false, capturing = false, supported = true;
    std::chrono::steady_clock::time_point tick_start;
    double duration = 0;
    unsigned long logical_tick = 0, serial = 0;
    std::vector<unsigned char> background;
    std::vector<Layer> layers;
    std::vector<std::array<unsigned char, 256>> effects;
};
extern Frame frame;
void Begin(GraphicViewPortClass& view, int x, int y, int width, int height);
void Begin_Object(double sx, double sy, double ex, double ey);
void End_Object();
void Begin_Foreground();
void Finish();
void Invalidate();
// Called by the existing rasterizers, recording coverage even when foreground and background colours match.
void Mark(const unsigned char* destination, int width, int height, int pitch,
          const unsigned char* source = nullptr, int source_pitch = 0, bool transparent = false,
          const unsigned char* ghost_lookup = nullptr, const unsigned char* ghost_table = nullptr,
          const unsigned char* fade_table = nullptr, int fade_count = 0);
// Palette-table effects are evaluated against the newly composed background, never a baked old position.
const std::vector<uint32_t>& Compose(int width, int height, const uint32_t* palette, double fraction);
}
#endif
