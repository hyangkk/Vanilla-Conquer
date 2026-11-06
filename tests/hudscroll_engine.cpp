// Actual native map-scroll and compositor regression using a private user-save copy.
#include "../tests/samtest_support.h"
#include "enemyfusion.h"
#include "smoothmotion.h"
#include "common/settings.h"
#include "common/renderlayers.h"
#include "common/filepcx.h"
static void HUD_Capture(const std::string& file){
 GraphicBufferClass image(HidPage.Get_Width(),HidPage.Get_Height(),nullptr,HidPage.Get_Width()*HidPage.Get_Height());HidPage.Blit(image);Write_PCX_File(const_cast<char*>(file.c_str()),image,GamePalette);
}
static std::vector<unsigned char> HUD_Pixels(){
 HidPage.Lock();auto ptr=reinterpret_cast<unsigned char*>(HidPage.Get_Offset());int pitch=HidPage.Get_Width()+HidPage.Get_XAdd()+HidPage.Get_Pitch();std::vector<unsigned char> pixels(HidPage.Get_Width()*HidPage.Get_Height());
 for(int y=0;y<HidPage.Get_Height();++y)std::memcpy(pixels.data()+y*HidPage.Get_Width(),ptr+y*pitch,HidPage.Get_Width());HidPage.Unlock();return pixels;
}
static std::array<std::uint32_t,256> HUD_Palette(){
 std::array<std::uint32_t,256> palette{};for(int i=0;i<256;++i){unsigned char rgba[4]={static_cast<unsigned char>(GamePalette[i*3]*4),static_cast<unsigned char>(GamePalette[i*3+1]*4),static_cast<unsigned char>(GamePalette[i*3+2]*4),255};std::memcpy(&palette[i],rgba,4);}return palette;
}
static void HUD_Composite_Capture(const std::string& file,const std::vector<std::uint32_t>& pixels){
 std::ofstream out(file,std::ios::binary);out<<"P6\n"<<HidPage.Get_Width()<<" "<<HidPage.Get_Height()<<"\n255\n";for(auto p:pixels)out.write(reinterpret_cast<const char*>(&p),3);
}
static bool HUD_Inside(int x,int y){return x>=WindowList[WINDOW_TACTICAL][WINDOWX]+6&&x<WindowList[WINDOW_TACTICAL][WINDOWX]+136&&y>=WindowList[WINDOW_TACTICAL][WINDOWY]+6&&y<WindowList[WINDOW_TACTICAL][WINDOWY]+138;}
struct HUD_Result{std::vector<unsigned char> raster;std::vector<std::uint32_t> composite;};
static HUD_Result HUD_Run(const std::string& dir,bool visible,bool smooth,bool moving){
 Check("load actual user campaign save copy",Load_Game((dir+"/user-current.sav").c_str()));Options.Set();GameToPlay=GAME_NORMAL;GameActive=true;GameInFocus=true;Settings.Video.SmoothMotion=smooth;Smooth_Motion_Reset();
 for(int i=0;i<Triggers.Count();++i)Triggers.Ptr(i)->Action=TriggerClass::ACTION_NONE;
 UnitClass* mover=nullptr;COORDINATE firstPosition=0;
 if(moving){
  for(int i=0;i<Units.Count();++i){auto u=Units.Ptr(i);if(u->IsActive&&!u->IsInLimbo&&u->House==PlayerPtr&&(*u==UNIT_MTANK||*u==UNIT_LTANK)){mover=u;break;}}
  if(!mover){mover=new UnitClass(UNIT_MTANK,PlayerPtr->Class->House);Check("place private native player tank",Put(mover,Center()));}
  Check("native player tank available for movement",mover!=nullptr);
  if(mover){firstPosition=mover->Coord;CELL destination=Coord_Cell(firstPosition);bool found=false;
   for(int radius=3;radius<8&&!found;++radius)for(int dy=-radius;dy<=radius&&!found;++dy)for(int dx=-radius;dx<=radius;++dx){if(MAX(ABS(dx),ABS(dy))!=radius)continue;int x=Cell_X(destination)+dx,y=Cell_Y(destination)+dy;if(x<0||x>=MAP_CELL_W||y<0||y>=MAP_CELL_H)continue;CELL cell=XY_Cell(x,y);if(Map.In_Radar(cell)&&mover->Can_Enter_Cell(cell)==MOVE_OK){mover->Assign_Mission(MISSION_MOVE);mover->Commence();mover->Assign_Destination(::As_Target(cell));found=true;break;}}
   Check("issue a real native tank movement order",found);
  }
 }
 ++ScenarioInit;Map.Set_Tactical_Position(Cell_Coord(XY_Cell(Map.MapCellX+10,Map.MapCellY+10)));--ScenarioInit;
 auto palette=HUD_Palette();auto old=Set_Logic_Page(HidPage);GameActive=visible;Map.Draw_It(true);RenderLayers::Finish();
 int steps=0;bool stable=true;DirType directions[]={DIR_NW,DIR_SE,DIR_SW,DIR_NE,DIR_E,DIR_W,DIR_N,DIR_S};
 for(auto direction:directions)for(int i=0;i<8;++i){
  GameActive=true;Smooth_Motion_Begin_Tick(4);++Frame;if(mover)mover->AI();Smooth_Motion_End_Tick();
  int distance=ICON_LEPTON_W;if(Map.Scroll_Map(direction,distance,true))++steps;
  GameActive=visible;Map.Draw_It(Smooth_Motion_Render_Enabled());RenderLayers::Finish();
  if(smooth&&visible){
   auto native=HUD_Pixels();
   for(double fraction:{0.,.5,1.}){auto composed=RenderLayers::Compose(HidPage.Get_Width(),HidPage.Get_Height(),palette.data(),fraction);
    for(int y=WindowList[WINDOW_TACTICAL][WINDOWY]+6;y<WindowList[WINDOW_TACTICAL][WINDOWY]+138;++y)
     for(int x=WindowList[WINDOW_TACTICAL][WINDOWX]+6;x<WindowList[WINDOW_TACTICAL][WINDOWX]+136;++x)
      if(composed[y*HidPage.Get_Width()+x]!=palette[native[y*HidPage.Get_Width()+x]]){stable=false;}
   }
  }
 }
 if(moving)Check("the actual tank moved while the map scrolled",mover&&mover->Coord!=firstPosition);
 if(smooth&&visible)Check("HUD stays fixed through all three interpolation fractions on every frame",stable);
 const std::string mode=smooth?(moving?"moving-smooth":"smooth"):"native";
 HUD_Capture(dir+"/"+mode+(visible?"-after-scroll.pcx":"-plain-scroll.pcx"));
 HUD_Result result;result.raster=HUD_Pixels();
 if(smooth){result.composite=RenderLayers::Compose(HidPage.Get_Width(),HidPage.Get_Height(),palette.data(),.5);if(visible)HUD_Composite_Capture(dir+"/"+mode+"-composite.ppm",result.composite);}
 if(visible){EnemyFusion::Restore_Counts();auto world=HUD_Pixels();EnemyFusion::Draw_Counts();EnemyFusion::Restore_Counts();Check("drawing and restoring the HUD preserves every world pixel",world==HUD_Pixels());EnemyFusion::Draw_Counts();}
 Set_Logic_Page(old);GameActive=true;std::printf("HUD_SCROLL smooth=%d visible=%d moving=%d steps=%d\n",int(smooth),int(visible),int(moving),steps);return result;
}
static void HUD_Compare(const HUD_Result& baseline,const HUD_Result& withHud,bool smooth){
 int raster=0,composite=0;int left=WindowList[WINDOW_TACTICAL][WINDOWX],top=WindowList[WINDOW_TACTICAL][WINDOWY],width=WindowList[WINDOW_TACTICAL][WINDOWWIDTH],height=WindowList[WINDOW_TACTICAL][WINDOWHEIGHT],pitch=HidPage.Get_Width();
 for(int y=top;y<top+height;++y)for(int x=left;x<left+width;++x){if(HUD_Inside(x,y))continue;int n=y*pitch+x;if(withHud.raster[n]!=baseline.raster[n])++raster;if(smooth&&withHud.composite[n]!=baseline.composite[n])++composite;}
 std::printf("HUD_AB smooth=%d extra_raster_pixels=%d extra_composite_pixels=%d\n",int(smooth),raster,composite);
 Check("native scroll introduces zero HUD pixels outside its fixed rectangle",raster==0);if(smooth)Check("final interpolated output introduces zero HUD pixels outside its fixed rectangle",composite==0);
}
bool Generate_Mission_Starts(const char* directory){
 std::string dir(directory);for(bool smooth:{false,true}){auto baseline=HUD_Run(dir,false,smooth,false);auto withHud=HUD_Run(dir,true,smooth,false);HUD_Compare(baseline,withHud,smooth);}
 HUD_Run(dir,true,true,true);std::printf("TOTAL FAILURES %d\n",failures);return failures==0;
}
