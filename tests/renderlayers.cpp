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
    gb.Unlock();
    std::puts("raster layer reconstruction and coverage: all checks passed");
}
