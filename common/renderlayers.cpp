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
#include <cmath>
#include <map>
#include <unordered_map>

namespace RenderLayers {
Frame frame;
namespace {
unsigned char* canvas = nullptr;
int canvas_pitch = 0;
bool recording = false;
bool foreground = false;
std::vector<unsigned char> coverage, before_foreground;
std::vector<unsigned short> materials;
std::map<std::array<unsigned char,256>, unsigned short> effect_ids;
std::unordered_map<unsigned int, unsigned short> composed_effects;
unsigned short Effect_ID(const std::array<unsigned char,256>& table)
{
    auto found = effect_ids.find(table);
    if (found != effect_ids.end()) return found->second;
    if (frame.effects.size() >= 65535) { frame.supported = false; return 0; }
    frame.effects.push_back(table);
    unsigned short id = static_cast<unsigned short>(frame.effects.size());
    effect_ids.emplace(table, id);
    return id;
}
int left, top, right, bottom;
double start_x, start_y, end_x, end_y;
unsigned int object_key;
double anchor_x, anchor_y;
bool selected;
std::vector<MotionInterpolation::Point> object_timeline;

void Clear_Mask()
{
    std::fill(coverage.begin(), coverage.end(), 0);
    std::fill(materials.begin(), materials.end(), 0);
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
    layer.object = object_key; layer.anchor_x = anchor_x; layer.anchor_y = anchor_y; layer.selected = selected; layer.timeline = object_timeline;
    layer.pixels.resize(layer.width * layer.height);
    layer.mask.resize(layer.pixels.size());
    layer.effects.resize(layer.pixels.size());
    for (int y = 0; y < layer.height; ++y) {
        std::memcpy(layer.pixels.data() + y * layer.width, canvas + (y + top) * canvas_pitch + left, layer.width);
        std::memcpy(layer.mask.data() + y * layer.width, coverage.data() + (y + top) * frame.width + left, layer.width);
        std::memcpy(layer.effects.data() + y * layer.width, materials.data() + (y + top) * frame.width + left,
                    layer.width * sizeof(unsigned short));
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
    frame.supported = true;
    frame.effects.clear(); effect_ids.clear(); composed_effects.clear();
    frame.width = view.Get_Width(); frame.height = view.Get_Height();
    frame.clip_x = x; frame.clip_y = y; frame.clip_w = width; frame.clip_h = height;
    canvas = reinterpret_cast<unsigned char*>(view.Get_Offset());
    canvas_pitch = view.Get_Pitch() + view.Get_XAdd() + view.Get_Width();
    coverage.resize(frame.width * frame.height);
    materials.resize(coverage.size());
    frame.layers.clear();
    recording = foreground = false;
    Snapshot(frame.background);
}
void Begin_Object(double sx, double sy, double ex, double ey, unsigned int object,
                  double ax, double ay, bool is_selected, const std::vector<MotionInterpolation::Point>* timeline)
{
    if (!frame.capturing || foreground) return;
    Clear_Mask();
    start_x = sx; start_y = sy; end_x = ex; end_y = ey;
    object_key = object; anchor_x = ax; anchor_y = ay; selected = is_selected;
    object_timeline = timeline ? *timeline : std::vector<MotionInterpolation::Point>();
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
    object_key = 0; anchor_x = anchor_y = 0; selected = false; object_timeline.clear();
    recording = foreground = true;
}
void Mark(const unsigned char* destination, int width, int height, int pitch,
          const unsigned char* source, int source_pitch, bool transparent,
          const unsigned char* ghost_lookup, const unsigned char* ghost_table,
          const unsigned char* fade_table, int fade_count)
{
    if (!recording || width <= 0 || height <= 0) return;
    const uintptr_t base = reinterpret_cast<uintptr_t>(canvas);
    const uintptr_t address = reinterpret_cast<uintptr_t>(destination);
    if (address < base || address >= base + canvas_pitch * frame.height) return;
    const int offset = int(address - base);
    const int ox = offset % canvas_pitch, oy = offset / canvas_pitch;
    if (pitch != canvas_pitch) return;
    unsigned short sprite_effects[256] = {};
    if (source && ghost_lookup && ghost_table) {
        for (int i = 0; i < 256; ++i) {
            if (ghost_lookup[i] == 0xFF) continue;
            std::array<unsigned char,256> table;
            for (int c = 0; c < 256; ++c) {
                unsigned char value = ghost_table[ghost_lookup[i]*256+c];
                for (int f = 0; fade_table && f < fade_count; ++f) value = fade_table[value];
                table[c] = value;
            }
            sprite_effects[i] = Effect_ID(table);
        }
    }
    for (int y = 0; y < height && oy + y < frame.height; ++y) {
        for (int x = 0; x < width && ox + x < frame.width; ++x) {
            if (transparent && source && !source[y * source_pitch + x]) continue;
            const int position = (oy + y) * frame.width + ox + x;
            unsigned short effect = source ? sprite_effects[source[y*source_pitch+x]] : 0;
            if (effect && coverage[position]) {
                const unsigned short previous = materials[position];
                if (!previous) effect = 0; // Effect over this layer's own opaque pixel is already resolved.
                else {
                    unsigned int key = (unsigned(effect)<<16) | previous;
                    auto found = composed_effects.find(key);
                    if (found != composed_effects.end()) effect = found->second;
                    else {
                        std::array<unsigned char,256> table;
                        for (int c=0;c<256;++c) table[c] = frame.effects[effect-1][frame.effects[previous-1][c]];
                        effect = Effect_ID(table); composed_effects.emplace(key, effect);
                    }
                }
            }
            materials[position] = effect;
            coverage[position] = 1;
            left = std::min(left, ox + x); right = std::max(right, ox + x);
            top = std::min(top, oy + y); bottom = std::max(bottom, oy + y);
        }
    }
}
void Clear_Details() { frame.details.clear(); ++frame.serial; }
bool Detail_Visible(const Detail& detail, const unsigned char* pixels, int width, int height, int pitch)
{
    if (!pixels || detail.x < 0 || detail.y < 0 || detail.width <= 0 || detail.height <= 0 ||
        detail.x + detail.width > width || detail.y + detail.height > height ||
        detail.reference.size() != size_t(detail.width * detail.height)) return false;
    for (int row = 0; row < detail.height; ++row)
        if (std::memcmp(detail.reference.data() + row * detail.width,
                        pixels + (detail.y + row) * pitch + detail.x, detail.width) != 0) return false;
    return true;
}
void Add_Detail(GraphicViewPortClass& source, int source_width, int source_height,
                GraphicViewPortClass& destination, int x, int y, int width, int height)
{
    auto existing = std::find_if(frame.details.begin(), frame.details.end(),
        [&](const Detail& detail) { return detail.x == x && detail.y == y; });
    if ((existing == frame.details.end() && frame.details.size() >= 32) ||
        x < 0 || y < 0 || width <= 0 || height <= 0 ||
        x + width > destination.Get_Width() || y + height > destination.Get_Height() ||
        source_width <= 0 || source_height <= 0 ||
        source_width > source.Get_Width() || source_height > source.Get_Height()) return;
    Detail detail{x, y, width, height, source_width, source_height, {}, {}};
    if (!source.Lock()) return;
    auto pixels = reinterpret_cast<const unsigned char*>(source.Get_Offset());
    const int pitch = source.Get_Width() + source.Get_XAdd() + source.Get_Pitch();
    detail.pixels.resize(source_width * source_height);
    for (int row = 0; row < source_height; ++row)
        std::memcpy(detail.pixels.data() + row * source_width, pixels + row * pitch, source_width);
    source.Unlock();
    if (!destination.Lock()) return;
    const auto target = reinterpret_cast<const unsigned char*>(destination.Get_Offset());
    const int target_pitch = destination.Get_Width() + destination.Get_XAdd() + destination.Get_Pitch();
    detail.reference.resize(width * height);
    for (int row = 0; row < height; ++row)
        std::memcpy(detail.reference.data() + row * width, target + (y + row) * target_pitch + x, width);
    destination.Unlock();
    if (existing == frame.details.end()) frame.details.push_back(std::move(detail));
    else *existing = std::move(detail);
    ++frame.serial;
}
void Finish()
{
    if (!frame.capturing || !foreground) return;
    // Font/gadget rasterizers can bypass sprite and line coverage; include their changed pixels too.
    for (int y = 0; y < frame.height; ++y) {
        for (int x = 0; x < frame.width; ++x) {
            if (!coverage[y*frame.width+x] && canvas[y * canvas_pitch + x] != before_foreground[y * frame.width + x])
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
    frame.ready = frame.supported;
}
void Offset(const Layer& layer, double fraction, double& x, double& y)
{
    if (!layer.timeline.empty()) {
        const double start=std::chrono::duration<double>(frame.tick_start.time_since_epoch()).count();
        const double time=(frame.presentation_time>0 ? frame.presentation_time : start+fraction*frame.duration)-frame.motion_delay;
        const auto point=MotionInterpolation::Smooth_Timeline(layer.timeline,time,frame.duration);
        x=point.x-layer.anchor_x; y=point.y-layer.anchor_y;
    } else {
        x=layer.start_x+(layer.end_x-layer.start_x)*fraction;
        y=layer.start_y+(layer.end_y-layer.start_y)*fraction;
    }
}

const std::vector<uint32_t>& Compose(int width, int height, const uint32_t* palette, double fraction)
{
    static std::vector<uint32_t> background, output;
    static std::array<uint32_t,256> last_palette = {};
    static unsigned long last_serial = 0;
    static int last_width = 0, last_height = 0;
    static std::unordered_map<uint32_t,unsigned char> nearest;
    const bool palette_changed = std::memcmp(last_palette.data(), palette, 256*sizeof(uint32_t)) != 0;
    if (palette_changed || nearest.size()>65536) {
        std::copy(palette, palette+256, last_palette.begin());
        nearest.clear();
        for (int c=0;c<256;++c) nearest.emplace(palette[c], static_cast<unsigned char>(c));
    }
    if (last_serial != frame.serial || width != last_width || height != last_height || palette_changed) {
        background.resize(width*height);
        for (int y=0;y<height;++y) for (int x=0;x<width;++x)
            background[y*width+x] = palette[frame.background[(y*frame.height/height)*frame.width+x*frame.width/width]];
        for (const auto& detail : frame.details) {
            if (detail.x + detail.width > frame.width || detail.y + detail.height > frame.height) continue;
            // A menu, tooltip, or other UI may have covered the cameo since capture.
            // Preserve that final UI rather than painting artwork over it.
            if (!Detail_Visible(detail, frame.background.data(), frame.width, frame.height, frame.width)) continue;
            const int left = (detail.x * width + frame.width - 1) / frame.width;
            const int top = (detail.y * height + frame.height - 1) / frame.height;
            const int right = ((detail.x + detail.width) * width + frame.width - 1) / frame.width;
            const int bottom = ((detail.y + detail.height) * height + frame.height - 1) / frame.height;
            for (int y = top; y < bottom; ++y) for (int x = left; x < right; ++x) {
                int u = std::min(detail.source_width - 1,
                    (x * frame.width - detail.x * width) * detail.source_width / (detail.width * width));
                int v = std::min(detail.source_height - 1,
                    (y * frame.height - detail.y * height) * detail.source_height / (detail.height * height));
                background[y * width + x] = palette[detail.pixels[v * detail.source_width + u]];
            }
        }
        last_serial = frame.serial; last_width = width; last_height = height;
    }
    output = background;
    const double sx = double(width)/frame.width, sy = double(height)/frame.height;
    auto Channel = [](uint32_t color, int channel) { return int(reinterpret_cast<const unsigned char*>(&color)[channel]); };
    auto Lookup = [&](uint32_t color) {
        auto found = nearest.find(color);
        if (found != nearest.end()) return found->second;
        int best=0, distance=0x7fffffff;
        for (int i=0;i<256;++i) {
            int dr=Channel(color,0)-Channel(palette[i],0), dg=Channel(color,1)-Channel(palette[i],1), db=Channel(color,2)-Channel(palette[i],2);
            int d=dr*dr+dg*dg+db*db;
            if (d<distance) { best=i; distance=d; }
        }
        unsigned char index=static_cast<unsigned char>(best);
        nearest.emplace(color,index);
        return index;
    };
    fraction = std::max(0.0,std::min(1.0,fraction));
    for (const auto& layer: frame.layers) {
        double offset_x, offset_y; Offset(layer,fraction,offset_x,offset_y);
        const double dx=offset_x*sx, dy=offset_y*sy;
        const double origin_x=layer.x*sx+dx, origin_y=layer.y*sy+dy;
        int left=std::max(int(std::floor(origin_x))-1,int(std::ceil(frame.clip_x*sx)));
        int top=std::max(int(std::floor(origin_y))-1,int(std::ceil(frame.clip_y*sy)));
        int right=std::min(int(std::ceil(origin_x+layer.width*sx))+1,int((frame.clip_x+frame.clip_w)*sx));
        int bottom=std::min(int(std::ceil(origin_y+layer.height*sy))+1,int((frame.clip_y+frame.clip_h)*sy));
        left=std::max(0,left); top=std::max(0,top); right=std::min(width,right); bottom=std::min(height,bottom);
        if (dx==0 && dy==0 && sx==std::floor(sx) && sy==std::floor(sy)) {
            for (int y=top;y<bottom;++y) for (int x=left;x<right;++x) {
                int px=int(std::floor((x+.5-origin_x)/sx)), py=int(std::floor((y+.5-origin_y)/sy));
                if (px<0 || px>=layer.width || py<0 || py>=layer.height) continue;
                int n=py*layer.width+px;
                if (!layer.mask[n]) continue;
                auto& destination=output[y*width+x];
                unsigned short effect=layer.effects[n];
                destination=effect ? palette[frame.effects[effect-1][Lookup(destination)]] : palette[layer.pixels[n]];
            }
            continue;
        }
        for (int y=top;y<bottom;++y) for (int x=left;x<right;++x) {
            double u=x-origin_x, v=y-origin_y;
            int ix=int(std::floor(u)), iy=int(std::floor(v));
            double fx=u-ix, fy=v-iy;
            uint32_t destination=output[y*width+x];
            double red=0,green=0,blue=0;
            for (int j=0;j<2;++j) for (int i=0;i<2;++i) {
                double weight=(i?fx:1-fx)*(j?fy:1-fy);
                if (weight==0) continue;
                int px=int(std::floor((ix+i+.5)/sx)), py=int(std::floor((iy+j+.5)/sy));
                uint32_t color=destination;
                if (px>=0 && px<layer.width && py>=0 && py<layer.height) {
                    int n=py*layer.width+px;
                    if (layer.mask[n]) {
                        unsigned short effect=layer.effects[n];
                        color=effect ? palette[frame.effects[effect-1][Lookup(destination)]] : palette[layer.pixels[n]];
                    }
                }
                red+=Channel(color,0)*weight; green+=Channel(color,1)*weight; blue+=Channel(color,2)*weight;
            }
            unsigned char rgba[4]={static_cast<unsigned char>(std::lround(red)),static_cast<unsigned char>(std::lround(green)),
                                   static_cast<unsigned char>(std::lround(blue)),255};
            std::memcpy(&output[y*width+x],rgba,4);
        }
    }
    return output;
}

}
