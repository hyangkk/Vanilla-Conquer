// Native regression: removed density volleys stay removed after loading old credits.
#define Generate_Mission_Starts Blast_Regression
#include "../tests/samblast_engine.cpp"
#undef Generate_Mission_Starts
static void NoExtras(const std::string& dir,int enemies){
 Reset(dir);auto sam=Source(true);Actors(false,MAX(enemies,1));sam->Coord=XY_Coord(Coord_X(As_Techno(actors[0])->Coord)-8*ICON_LEPTON_W,Coord_Y(As_Techno(actors[0])->Coord));
 for(int salvo=0;salvo<5;++salvo){auto b=static_cast<TechnoClass*>(sam)->Fire_At(actors[0],0);Check("nearby enemy count never adds missiles",b&&Bullets.Count()==1&&b->Strength==50);for(int i=Bullets.Count()-1;i>=0;--i)delete Bullets.Ptr(i);}
 Checkpoint(dir);sam=Buildings.Ptr(Buildings.ID(sam));sam=static_cast<BuildingClass*>(As_Techno(sam->As_Target()));auto b=static_cast<TechnoClass*>(sam)->Fire_At(actors[0],0);Check("save reload does not restore additional missiles",b&&Bullets.Count()==1);
}
static void OldCredit(const std::string& dir,char version){
 Reset(dir);auto sam=Source(true);Actors(false,20);sam->Coord=XY_Coord(Coord_X(As_Techno(actors[0])->Coord)-8*ICON_LEPTON_W,Coord_Y(As_Techno(actors[0])->Coord));
 CDFileClass file;char magic[8]={'C','N','C','S','A','M','0',version};std::vector<unsigned char> credits(BUILDING_MAX,0);credits[Buildings.ID(sam)]=version=='1'?4:9;
 Check("open old credit fixture",file.Open((dir+"/credit.bin").c_str(),WRITE));file.Write(magic,8);file.Write(credits.data(),credits.size());if(version=='3'){std::vector<unsigned char> hp(BUILDING_MAX,0);file.Write(hp.data(),hp.size());}file.Close();Check("read old credit fixture",file.Open((dir+"/credit.bin").c_str(),READ));Check("old density footer remains loadable",Campaign_SAM_Volley_Load(file));file.Close();static_cast<TechnoClass*>(sam)->Fire_At(actors[0],0);Check("legacy saved credits never emit extra missiles",Bullets.Count()==1);
}
bool Generate_Mission_Starts(const char* directory){std::string dir(directory);Blast_Regression(directory);for(int n:{1,2,5,10,20,50})NoExtras(dir,n);for(char v:{'1','2','3'})OldCredit(dir,v);std::printf("TOTAL DENSITY FAILURES %d\n",failures);return failures==0;}
