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
#include "function.h"
#include "smoothmotion.h"
#include "common/settings.h"
#include "common/renderlayers.h"
#include "common/motioninterpolation.h"
#include <chrono>
#include <unordered_map>

namespace {
using Clock = std::chrono::steady_clock;
struct Sample { COORDINATE previous; COORDINATE current; unsigned long generation; };
std::unordered_map<const ObjectClass*, Sample> samples;
Clock::time_point tick_start;
double tick_duration = 0.0;
unsigned long generation = 0;
bool enabled = false;

bool Mobile(const ObjectClass* object)
{
    const RTTIType type = object->What_Am_I();
    return type == RTTI_INFANTRY || type == RTTI_UNIT || type == RTTI_AIRCRAFT || type == RTTI_BULLET;
}
}

void Smooth_Motion_Reset()
{
    samples.clear();
    enabled = false;
    RenderLayers::Invalidate();
}

void Smooth_Motion_Forget(const ObjectClass* object)
{
    samples.erase(object);
}

void Smooth_Motion_Begin_Tick(int timer_ticks)
{
    // The remaster owns its renderer; leave its code path completely unchanged.
#if defined(REMASTER_BUILD) || !defined(SDL2_BUILD)
    return;
#endif
    enabled = Settings.Video.SmoothMotion && timer_ticks > 0 && !Debug_Map && !ScenarioInit;
    if (!enabled) {
        samples.clear();
            return;
    }
    tick_start = Clock::now();
    tick_duration = timer_ticks / 60.0;
    ++generation;
    for (LayerType layer = LAYER_GROUND; layer < LAYER_COUNT; layer++) {
        for (int i = 0; i < Map.Layer[layer].Count(); ++i) {
            ObjectClass* object = Map.Layer[layer][i];
            if (Mobile(object) && !object->IsInLimbo && object->IsDown) {
                Sample& sample = samples[object];
                sample.previous = sample.current = object->Render_Coord();
                sample.generation = generation;
            }
        }
    }
}

void Smooth_Motion_End_Tick()
{
    for (auto it = samples.begin(); it != samples.end();) {
        // Destruction erases entries, and scenario/load reset clears the entire sidecar.
        if (it->second.generation != generation || !it->first->IsDown || it->first->IsInLimbo) {
            it = samples.erase(it);
            continue;
        }
        Sample& sample = it->second;
        sample.current = it->first->Render_Coord();
        // Spawning, teleportation and large discontinuities must snap, never sweep across the map.
        if (!MotionInterpolation::Continuous(Coord_X(sample.previous), Coord_Y(sample.previous),
                                             Coord_X(sample.current), Coord_Y(sample.current), ICON_LEPTON_W)) {
            sample.previous = sample.current;
        }
        ++it;
    }
    if (enabled) {
        Map.Flag_To_Redraw(true);
    }
}

void Smooth_Motion_Scene_Begin()
{
    // Publish timing together with the new raster layers, never retime the previous scene.
    RenderLayers::frame.tick_start = tick_start;
    RenderLayers::frame.duration = tick_duration;
    RenderLayers::frame.logical_tick = Frame;
}

bool Smooth_Motion_Render_Enabled()
{
#ifdef SDL2_BUILD
    return enabled && SpecialDialog == SDLG_NONE && GameInFocus;
#else
    return false;
#endif
}

void Smooth_Motion_Object_Begin(const ObjectClass* object, unsigned int coordinate)
{
    double sx = 0, sy = 0, ex = 0, ey = 0;
    const auto it = samples.find(object);
    if (it != samples.end() && it->second.current == coordinate) {
        const Sample& sample = it->second;
        // Raster positions are rounded in Coord_To_Pixel; retain their fractional remainder on the GPU.
        const double scale = double(ICON_PIXEL_W) / ICON_LEPTON_W;
        const int x = Lepton_To_Pixel(Coord_X(coordinate));
        const int y = Lepton_To_Pixel(Coord_Y(coordinate));
        sx = Coord_X(sample.previous) * scale - x;
        sy = Coord_Y(sample.previous) * scale - y;
        ex = Coord_X(coordinate) * scale - x;
        ey = Coord_Y(coordinate) * scale - y;
    }
    RenderLayers::Begin_Object(sx, sy, ex, ey);
}
void Smooth_Motion_Object_End()
{
    RenderLayers::End_Object();
}
