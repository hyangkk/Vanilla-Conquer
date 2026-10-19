#include "common/drawline.h"
#include "common/gbuffer.h"
#include "common/wwkeyboard.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <iostream>

// Globals needed to compile GraphicBufferClass.
bool GameInFocus;
int ScreenWidth;
int WindowList[9][9];
extern "C" char* _ShapeBuffer = 0;
WWKeyboardClass* Keyboard;

void Process_Network()
{
}

void Focus_Restore()
{
}

void Focus_Loss()
{
}

int Open_File(char const*, int)
{
    return 0;
}

void Close_File(int)
{
}

int Read_File(int, void*, unsigned int)
{
    return 0;
}

#include "common/renderlayers.h"
#include <cstdlib>
#include <vector>
static void Check(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}
int main() {
    GraphicBufferClass gb(16, 16);
    Check(gb.Lock(), "CPU surface must lock");
    gb.Clear(7);
    RenderLayers::Begin(gb, 0, 0, 16, 16);
    RenderLayers::Begin_Object(-.75, 0, .25, 0);
    gb.Fill_Rect(2, 2, 4, 4, 7); // Same colour as background: a diff-only mask would lose these pixels.
    RenderLayers::End_Object();
    Check(RenderLayers::frame.layers.size()==1, "Identical-colour sprite coverage must survive");
    const auto& same=RenderLayers::frame.layers[0];
    Check(same.mask.size()==9 && same.mask[4]==1 && same.start_x==-.75, "Coverage and fractional offsets must survive");
    RenderLayers::Begin_Object(0, 0, 0, 0);
    gb.Draw_Line(1, 1, 12, 10, 19);
    gb.Put_Pixel(3, 4, 18);
    RenderLayers::End_Object();
    RenderLayers::Begin_Foreground();
    gb.Fill_Rect(8, 0, 15, 15, 0); // Solid fog must cover every pixel, including already-black pixels.
    RenderLayers::Begin_Object(1, 1, 1, 1); // Placement preview renders during foreground, without erasing fog coverage.
    gb.Put_Pixel(1, 14, 12);
    RenderLayers::End_Object();
    RenderLayers::Finish();
    Check(RenderLayers::frame.ready && RenderLayers::frame.layers.size()==3, "Foreground must be captured last");
    auto rebuilt=RenderLayers::frame.background;
    for (const auto& layer: RenderLayers::frame.layers)
        for(int y=0;y<layer.height;++y) for(int x=0;x<layer.width;++x) {
            int i=y*layer.width+x;
            if(layer.mask[i]) rebuilt[(layer.y+y)*16+layer.x+x]=layer.pixels[i];
        }
    Check(std::memcmp(rebuilt.data(), gb.Get_Buffer(), 256)==0, "Layer reconstruction must equal the original raster exactly");
    gb.Put_Pixel(0,0,33);
    Check(RenderLayers::frame.background[0]==7, "Published raster must be immutable");
    RenderLayers::frame.publishing=true; RenderLayers::Invalidate();
    Check(RenderLayers::frame.ready, "Publishing its own frame must not invalidate it");
    RenderLayers::frame.publishing=false; RenderLayers::Invalidate();
    Check(!RenderLayers::frame.ready, "A modal/menu blit must invalidate the game layers");
    uint32_t palette[256];
    for(int i=0;i<256;++i) { unsigned char rgba[4]={static_cast<unsigned char>(i),static_cast<unsigned char>(i),static_cast<unsigned char>(i),255}; std::memcpy(&palette[i],rgba,4); }
    for(int y=0;y<16;++y) for(int x=0;x<16;++x) gb.Put_Pixel(x,y,20+x*10);
    RenderLayers::Begin(gb,0,0,16,16);
    RenderLayers::Begin_Object(2,0,2,0);
    unsigned char source=1, lookup[256], table[256];
    std::memset(lookup,255,256); lookup[1]=0;
    for(int i=0;i<256;++i) table[i]=i/2;
    auto* canvas=static_cast<unsigned char*>(gb.Get_Buffer());
    RenderLayers::Mark(canvas+2*16+2,1,1,16,&source,1,true,lookup,table);
    canvas[2*16+2]=20; // Rasterized at x2 over colour40; presentation moves to x4 over colour60.
    RenderLayers::End_Object(); RenderLayers::Begin_Foreground(); RenderLayers::Finish();
    const auto shadow=RenderLayers::Compose(16,16,palette,0);
    Check(shadow[2*16+4]==palette[30] && shadow[2*16+2]==palette[40], "Moving shadow must remap its new background, not carry the old terrain");
    Check(RenderLayers::frame.effects.size()==1, "Identical palette effects must deduplicate");
    RenderLayers::Begin(gb,0,0,16,16); RenderLayers::Begin_Foreground();
    RenderLayers::Mark(canvas+3*16+3,1,1,16,&source,1,true,lookup,table);
    canvas[3*16+3]/=2;
    RenderLayers::Mark(canvas+3*16+3,1,1,16); canvas[3*16+3]=250; // Actual font writer overwrites an existing fog material.
    RenderLayers::Finish();
    Check(RenderLayers::Compose(16,16,palette,1)[3*16+3]==palette[250], "Foreground text must replace fog material with opaque ink");
    for(int y=0;y<16;++y) for(int x=0;x<16;++x) gb.Put_Pixel(x,y,20+x*10);
    RenderLayers::Begin(gb,0,0,16,16); RenderLayers::Begin_Object(0,0,1,0);
    gb.Put_Pixel(2,2,255); RenderLayers::End_Object(); RenderLayers::Begin_Foreground(); RenderLayers::Finish();
    const auto quarter=RenderLayers::Compose(16,16,palette,.25);
    const auto half=RenderLayers::Compose(16,16,palette,.5);
    Check(quarter[2*16+2]==palette[201] && half[2*16+2]==palette[148], "Subpixel output must change before a whole source pixel of movement");
    RenderLayers::Layer tracked;
    tracked.anchor_x=20; tracked.anchor_y=0;
    tracked.timeline={{0,0,0},{.05,10,0},{.133,20,0},{.183,30,0}};
    RenderLayers::frame.motion_delay=.1;
    RenderLayers::frame.presentation_time=.213;
    double ox,oy; RenderLayers::Offset(tracked,1,ox,oy);
    Check(std::abs((tracked.anchor_x+ox)-(10+(.113-.05)/(.133-.05)*10))<1e-6,
          "A late presentation must use its actual timestamp rather than the clamped previous tick phase");
    double position=tracked.anchor_x+ox;
    tracked.anchor_x=30; RenderLayers::Offset(tracked,0,ox,oy);
    Check(std::abs(tracked.anchor_x+ox-position)<1e-6,
          "Replacing the scene raster must not jump the displayed world position");
    GraphicBufferClass detail(4,2);
    for(int y=0;y<2;++y) for(int x=0;x<4;++x) detail.Put_Pixel(x,y,40+y*4+x);
    gb.Fill_Rect(0,0,15,15,7);
    RenderLayers::Clear_Details();
    // Sidebar redraws can happen before the first interpolated world frame.
    RenderLayers::Add_Detail(detail,4,2,gb,8,2,2,1);
    RenderLayers::Invalidate(); // Fastest speed has no interpolated world frame.
    const auto* ui = reinterpret_cast<const unsigned char*>(gb.Get_Offset());
    const int uiPitch = gb.Get_Width()+gb.Get_XAdd()+gb.Get_Pitch();
    Check(RenderLayers::Detail_Visible(RenderLayers::frame.details.front(),ui,16,16,uiPitch),
          "Native UI must remain available without a ready motion frame");
    gb.Put_Pixel(8,2,99);
    Check(!RenderLayers::Detail_Visible(RenderLayers::frame.details.front(),ui,16,16,uiPitch),
          "Fallback UI must respect a menu covering the live front surface");
    gb.Put_Pixel(8,2,7);
    RenderLayers::Begin(gb,0,0,8,16); RenderLayers::Begin_Foreground();
    RenderLayers::Finish();
    const auto crisp=RenderLayers::Compose(32,32,palette,1);
    for(int y=0;y<2;++y) for(int x=0;x<4;++x)
        Check(crisp[(4+y)*32+16+x]==palette[40+y*4+x],
              "Compact UI must retain native artwork strokes lost by logical downsampling");
    RenderLayers::Begin(gb,0,0,8,16); RenderLayers::Begin_Foreground();
    RenderLayers::Add_Detail(detail,4,2,gb,8,2,2,1);
    gb.Put_Pixel(8,2,99); // A tooltip covers part of this cameo after its capture.
    RenderLayers::Finish();
    const auto covered=RenderLayers::Compose(32,32,palette,1);
    Check(covered[4*32+16]==palette[99] && covered[4*32+18]==palette[7],
          "Native UI artwork must not cover later tooltips or modal text");
    RenderLayers::Begin(gb,0,0,8,16);
    Check(RenderLayers::frame.details.size()==1, "Unchanged sidebar artwork must survive a new world frame");
    RenderLayers::Clear_Details();
    Check(RenderLayers::frame.details.empty(), "A mission load must discard old sidebar artwork");
    RenderLayers::Begin_Foreground(); RenderLayers::Finish();
    gb.Unlock();
    std::puts("raster layer reconstruction and coverage: all checks passed");
}
