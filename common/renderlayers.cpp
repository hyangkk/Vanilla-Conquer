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
#include "renderlayers.h"
#include "graphicsviewport.h"
#include <algorithm>
#include <cstring>

namespace RenderLayers {
Frame frame;
namespace {
unsigned char* canvas = nullptr;
int canvas_pitch = 0;
bool recording = false;
bool foreground = false;
std::vector<unsigned char> coverage, before_foreground;
int left, top, right, bottom;
double start_x, start_y, end_x, end_y;

void Clear_Mask()
{
    std::fill(coverage.begin(), coverage.end(), 0);
    left = frame.width; top = frame.height; right = bottom = -1;
}
void Snapshot(std::vector<unsigned char>& destination)
{
    destination.resize(frame.width * frame.height);
    for (int y = 0; y < frame.height; ++y)
        std::memcpy(destination.data() + y * frame.width, canvas + y * canvas_pitch, frame.width);
}
void Add_Layer()
{
    if (right < left || bottom < top) return;
    Layer layer;
    layer.x = left; layer.y = top; layer.width = right - left + 1; layer.height = bottom - top + 1;
    layer.start_x = start_x; layer.start_y = start_y; layer.end_x = end_x; layer.end_y = end_y;
    layer.pixels.resize(layer.width * layer.height);
    layer.mask.resize(layer.pixels.size());
    for (int y = 0; y < layer.height; ++y) {
        std::memcpy(layer.pixels.data() + y * layer.width, canvas + (y + top) * canvas_pitch + left, layer.width);
        std::memcpy(layer.mask.data() + y * layer.width, coverage.data() + (y + top) * frame.width + left, layer.width);
    }
    frame.layers.push_back(std::move(layer));
}
}
void Invalidate()
{
    if (!frame.publishing) frame.ready = false;
}
void Begin(GraphicViewPortClass& view, int x, int y, int width, int height)
{
    frame.ready = false; frame.capturing = true;
    ++frame.serial;
    frame.width = view.Get_Width(); frame.height = view.Get_Height();
    frame.clip_x = x; frame.clip_y = y; frame.clip_w = width; frame.clip_h = height;
    canvas = reinterpret_cast<unsigned char*>(view.Get_Offset());
    canvas_pitch = view.Get_Pitch() + view.Get_XAdd() + view.Get_Width();
    coverage.resize(frame.width * frame.height);
    frame.layers.clear();
    recording = foreground = false;
    Snapshot(frame.background);
}
void Begin_Object(double sx, double sy, double ex, double ey)
{
    if (!frame.capturing || foreground) return;
    Clear_Mask();
    start_x = sx; start_y = sy; end_x = ex; end_y = ey;
    recording = true;
}
void End_Object()
{
    if (!recording || foreground) return;
    recording = false;
    Add_Layer();
}
void Begin_Foreground()
{
    if (!frame.capturing) return;
    Snapshot(before_foreground);
    Clear_Mask();
    start_x = start_y = end_x = end_y = 0;
    recording = foreground = true;
}
void Mark(const unsigned char* destination, int width, int height, int pitch,
          const unsigned char* source, int source_pitch, bool transparent)
{
    if (!recording || width <= 0 || height <= 0) return;
    const uintptr_t base = reinterpret_cast<uintptr_t>(canvas);
    const uintptr_t address = reinterpret_cast<uintptr_t>(destination);
    if (address < base || address >= base + canvas_pitch * frame.height) return;
    const int offset = int(address - base);
    const int ox = offset % canvas_pitch, oy = offset / canvas_pitch;
    if (pitch != canvas_pitch) return;
    for (int y = 0; y < height && oy + y < frame.height; ++y) {
        for (int x = 0; x < width && ox + x < frame.width; ++x) {
            if (transparent && source && !source[y * source_pitch + x]) continue;
            coverage[(oy + y) * frame.width + ox + x] = 1;
            left = std::min(left, ox + x); right = std::max(right, ox + x);
            top = std::min(top, oy + y); bottom = std::max(bottom, oy + y);
        }
    }
}
void Finish()
{
    if (!frame.capturing || !foreground) return;
    // Font/gadget rasterizers can bypass sprite and line coverage; include their changed pixels too.
    for (int y = 0; y < frame.height; ++y) {
        for (int x = 0; x < frame.width; ++x) {
            if (canvas[y * canvas_pitch + x] != before_foreground[y * frame.width + x])
                Mark(canvas + y * canvas_pitch + x, 1, 1, canvas_pitch);
            // The sidebar/menu is never displaced and remains the original final raster.
            if (x < frame.clip_x || x >= frame.clip_x + frame.clip_w ||
                y < frame.clip_y || y >= frame.clip_y + frame.clip_h)
                frame.background[y * frame.width + x] = canvas[y * canvas_pitch + x];
        }
    }
    recording = false;
    Add_Layer();
    frame.capturing = foreground = false;
    frame.ready = true;
}
}
