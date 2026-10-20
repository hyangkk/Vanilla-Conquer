// Native private SAM damage/blast probe. Never ship this entry point.
#include "function.h"
#include "campaigndefense.h"
#include <cstdio>
#include <string>
#include <vector>
#include <fstream>
#include "campaignenemies.h"
static int failures;
static void Check(const char* name,bool ok){std::printf("%s %s\n",ok?"PASS":"FAIL",name);std::fflush(stdout);if(!ok)++failures;}
static void Reset(const std::string& dir){Check("load blast fixture",Load_Game((dir+"/fixture.sav").c_str()));Options.Set();GameToPlay=GAME_NORMAL;GameActive=true;PlayerWins=PlayerLoses=false;Frame=100;for(int i=0;i<Triggers.Count();++i)Triggers.Ptr(i)->Action=TriggerClass::ACTION_NONE;for(int i=Units.Count()-1;i>=0;--i)Units.Ptr(i)->Limbo();for(int i=Infantry.Count()-1;i>=0;--i)Infantry.Ptr(i)->Limbo();for(int i=Buildings.Count()-1;i>=0;--i)Buildings.Ptr(i)->Limbo();for(int i=Aircraft.Count()-1;i>=0;--i)Aircraft.Ptr(i)->Limbo();auto good=HouseClass::As_Pointer(HOUSE_GOOD),bad=HouseClass::As_Pointer(HOUSE_BAD);PlayerPtr=good;good->IsHuman=true;bad->IsHuman=false;good->FirepowerBias=bad->FirepowerBias=1;good->Power=bad->Power=1000000;}
static bool Put(TechnoClass* obj,CELL prefer){for(int radius=0;radius<10;++radius)for(int dy=-radius;dy<=radius;++dy)for(int dx=-radius;dx<=radius;++dx){if(MAX(ABS(dx),ABS(dy))!=radius)continue;int x=Cell_X(prefer)+dx,y=Cell_Y(prefer)+dy;if(x<0||x>=MAP_CELL_W||y<0||y>=MAP_CELL_H)continue;CELL cell=XY_Cell(x,y);if(Map.In_Radar(cell)&&obj->Can_Enter_Cell(cell)==MOVE_OK&&obj->Unlimbo(Cell_Coord(cell),DIR_N))return true;}return false;}
static CELL Center(){return XY_Cell(Map.MapCellX+Map.MapCellWidth/2,Map.MapCellY+Map.MapCellHeight/2);}
static BuildingClass* Source(bool player){auto s=new BuildingClass(STRUCT_SAM,player?HOUSE_GOOD:HOUSE_BAD);Check("place SAM away from blast",Put(s,XY_Cell(Map.MapCellX+2,Map.MapCellY+2)));return s;}
static void TickBullets(){std::vector<TARGET> ids;for(int i=0;i<Bullets.Count();++i)ids.push_back(Bullets.Ptr(i)->As_Target());++Frame;for(auto id:ids){auto b=As_Bullet(id);if(b)b->AI();}}

#include <set>
#include <cstdint>
#include <cstring>
static std::vector<TARGET> actors;
static void Actors(bool air=false,int count=13){
 actors.clear();auto c=Center();
 for(int i=0;i<count;++i){TechnoClass* a=air?static_cast<TechnoClass*>(new AircraftClass(AIRCRAFT_ORCA,HOUSE_BAD)):static_cast<TechnoClass*>(new UnitClass(UNIT_MTANK,HOUSE_BAD));
  if(air)static_cast<AircraftClass*>(a)->Altitude=AircraftClass::FLIGHT_LEVEL;
  Check("place chain target",Put(a,XY_Cell(Cell_X(c)+(i%4)*2,Cell_Y(c)+(i/4)*2)));a->Strength=30000;a->IsDiscoveredByPlayer=true;Campaign_Enemies_Mark_Extra(a);actors.push_back(a->As_Target());}
}
static int Loss(){int sum=0;for(auto id:actors){auto a=As_Techno(id);if(a)sum+=30000-a->Strength;}return sum;}
static TechnoClass* Aim(BulletClass* b){TechnoClass* best=nullptr;int distance=0x7fffffff;for(auto id:actors){auto a=As_Techno(id);if(!a||a->IsInLimbo)continue;int d=Distance(b->Fuse_Target(),a->Target_Coord());if(d<distance){best=a;distance=d;}}return best;}
static void Checkpoint(const std::string& dir){int count=Bullets.Count();Check("save ricochet position",Save_Game((dir+"/branch.sav").c_str(),"TEN SAM RICOCHETS"));Check("reload ricochet position",Load_Game((dir+"/branch.sav").c_str()));Check("checkpoint preserves each single travelling missile",Bullets.Count()==count);}
static void ImpactGeneration(bool hit){std::vector<TARGET> ids;for(int i=0;i<Bullets.Count();++i)ids.push_back(Bullets.Ptr(i)->As_Target());for(auto id:ids){auto b=As_Bullet(id);auto a=Aim(b);Check("generation has target",a!=nullptr);if(!a)continue;b->Coord=hit?a->Target_Coord():XY_Coord(Coord_X(a->Target_Coord()),Coord_Y(a->Target_Coord())+2*ICON_LEPTON_W);Check("player impact is handled without splash",Campaign_SAM_Chain_Impact(b,a));delete b;}}
static void ForcedChain(const std::string& dir,bool miss,bool bonus,int checkpoint){
 Reset(dir);auto sam=Source(true);Actors();if(bonus)sam->Coord=XY_Coord(Coord_X(As_Techno(actors[0])->Coord)-8*ICON_LEPTON_W,Coord_Y(As_Techno(actors[0])->Coord));
 auto root=static_cast<TechnoClass*>(sam)->Fire_At(actors[0],0);Check("root damage restored to stock fifty",root&&root->Strength==50);if(!root)return;
 if(bonus){Check("real density volley includes extra missiles",Bullets.Count()>1);BulletClass* extra=nullptr;for(int i=0;i<Bullets.Count();++i)if(Bullets.Ptr(i)!=root){extra=Bullets.Ptr(i);break;}if(!extra)return;for(int i=Bullets.Count()-1;i>=0;--i)if(Bullets.Ptr(i)!=extra)delete Bullets.Ptr(i);}
 std::printf("CHAIN miss=%d bonus=%d checkpoint=%d\n",int(miss),int(bonus),checkpoint);
 if(checkpoint==0)Checkpoint(dir);
 for(int generation=1;generation<=11;++generation){int before=Loss();ImpactGeneration(!miss);int expected=generation<=10?1:0;std::printf("GENERATION %d live=%d loss=%d\n",generation,Bullets.Count(),Loss());Check("each impact emits one missile up to ten ricochets then stops",Bullets.Count()==expected);if(generation==1)Check("root impact never applies splash",Loss()-before==(miss?0:50));for(int i=0;i<Bullets.Count();++i)Check("every branch retains stock raw damage",Bullets.Ptr(i)->Strength==50);if(checkpoint==generation&&generation<11)Checkpoint(dir);}
 Check("exactly eleven impacts or zero forced miss damage",Loss()==(miss?0:550));if(!miss){int distinct=0;for(auto id:actors)if(As_Techno(id)->Strength<30000)++distinct;Check("eleven-entry history avoids repeating targets while fresh enemies remain",distinct==11);}
}
static void SparseChain(const std::string& dir,int targets){
 Reset(dir);auto sam=Source(true);Actors(false,targets);sam->TechnoClass::Fire_At(actors[0],0);
 for(int attempt=1;attempt<=11;++attempt){ImpactGeneration(true);Check("sparse target reuse emits one missile through ten ricochets",Bullets.Count()==(attempt<=10?1:0));if(attempt==5)Checkpoint(dir);}
 Check("sparse targets receive eleven individual hits without a twelfth",Loss()==550&&Bullets.Count()==0);if(targets==2)Check("two remaining enemies alternate instead of hitting the same one repeatedly",As_Techno(actors[0])->Strength==29700&&As_Techno(actors[1])->Strength==29750);
}
static void NativeChain(const std::string& dir,bool air,bool sourceLost){
 Reset(dir);auto sam=Source(true);Actors(air);auto root=static_cast<TechnoClass*>(sam)->Fire_At(actors[0],0);Check("native tree starts",root!=nullptr);if(sourceLost){sam->Detach_All();delete sam;}
 for(int tick=0;tick<2500&&Bullets.Count();++tick)TickBullets();std::printf("NATIVE air=%d sourceLost=%d damage=%d remaining=%d\n",int(air),int(sourceLost),Loss(),Bullets.Count());
 Check("real flights deliver eleven stock-strength impacts",Loss()==11*(air?37:50));Check("real tree ends after ten ricochets",Bullets.Count()==0);
}
static void Boundary(const std::string& dir,bool outside,bool miss){
 Reset(dir);auto sam=Source(true);Actors(false,2);auto first=As_Techno(actors[0]),next=As_Techno(actors[1]);auto origin=XY_Coord(Coord_X(first->Target_Coord()),Coord_Y(first->Target_Coord())+(miss?2*ICON_LEPTON_W:0));next->Coord=XY_Coord(Coord_X(origin)+10*ICON_LEPTON_W+(outside?1:0),Coord_Y(origin));auto root=static_cast<TechnoClass*>(sam)->Fire_At(actors[0],0);root->Coord=origin;first->Limbo();Check("boundary impact handled",Campaign_SAM_Chain_Impact(root,nullptr));delete root;Check("ten-cell edge exact includes one continuation and plus one excludes all",Bullets.Count()==(outside?0:1));Check("no splash on next target",next->Strength==30000&&first->Strength==30000);
}
static void MixedTargets(const std::string& dir){
 Reset(dir);auto sam=Source(true);Actors(false,1);auto first=As_Techno(actors[0]);auto c=Coord_Cell(first->Coord);
 for(int type=0;type<3;++type){TechnoClass* a=type==0?static_cast<TechnoClass*>(new BuildingClass(STRUCT_POWER,HOUSE_BAD)):type==1?static_cast<TechnoClass*>(new InfantryClass(INFANTRY_E1,HOUSE_BAD)):static_cast<TechnoClass*>(new AircraftClass(AIRCRAFT_ORCA,HOUSE_BAD));if(type==2)static_cast<AircraftClass*>(a)->Altitude=AircraftClass::FLIGHT_LEVEL;Check("place mixed legal child target",Put(a,XY_Cell(Cell_X(c)+3+type*2,Cell_Y(c))));a->IsDiscoveredByPlayer=true;a->Strength=30000;actors.push_back(a->As_Target());}
 auto shot=static_cast<TechnoClass*>(sam)->Fire_At(actors[0],0);shot->Coord=first->Target_Coord();Campaign_SAM_Chain_Impact(shot,first);delete shot;Check("mixed target chain has only one missile",Bullets.Count()==1);std::set<TARGET> aims;for(int hop=0;hop<3&&Bullets.Count();++hop){auto a=Aim(Bullets.Ptr(0));if(a)aims.insert(a->As_Target());ImpactGeneration(true);}Check("single chain automatically visits building infantry and aircraft",aims.size()==3&&aims.count(actors[1])&&aims.count(actors[2])&&aims.count(actors[3]));
}
static void Exclusions(const std::string& dir){
 Reset(dir);auto sam=Source(true);Actors(false,1);auto first=As_Techno(actors[0]);auto c=Coord_Cell(first->Coord);
 HouseClass::As_Pointer(HOUSE_GOOD)->Make_Ally(HOUSE_JP);HouseClass::As_Pointer(HOUSE_JP)->Make_Ally(HOUSE_GOOD);
 for(auto house:{HOUSE_GOOD,HOUSE_JP,HOUSE_NEUTRAL}){auto a=new UnitClass(UNIT_MTANK,house);Put(a,c);a->IsDiscoveredByPlayer=true;}
 auto hidden=new UnitClass(UNIT_MTANK,HOUSE_BAD);Put(hidden,c);hidden->IsDiscoveredByPlayer=false;
 auto shot=static_cast<TechnoClass*>(sam)->Fire_At(actors[0],0);shot->Coord=first->Target_Coord();first->Limbo();Campaign_SAM_Chain_Impact(shot,first);delete shot;Check("own allied neutral hidden and limbo actors never receive branches",Bullets.Count()==0);
}
static void EdgeAndLostTarget(const std::string& dir,bool edge,bool bonus,bool outside=false){
 Reset(dir);auto sam=Source(true);Actors(false,13);if(bonus)sam->Coord=XY_Coord(Coord_X(As_Techno(actors[0])->Coord)-8*ICON_LEPTON_W,Coord_Y(As_Techno(actors[0])->Coord));auto root=static_cast<TechnoClass*>(sam)->Fire_At(actors[0],0);if(bonus){Check("edge bonus fixture",Bullets.Count()>1);for(int i=0;i<Bullets.Count();++i)if(Bullets.Ptr(i)!=root){root=Bullets.Ptr(i);break;}for(int i=Bullets.Count()-1;i>=0;--i)if(Bullets.Ptr(i)!=root)delete Bullets.Ptr(i);}
 COORDINATE origin=edge?XY_Coord(1,Coord_Y(As_Techno(actors[0])->Coord)):XY_Coord(Coord_X(As_Techno(actors[0])->Coord),Coord_Y(As_Techno(actors[0])->Coord)+2*ICON_LEPTON_W);
 if(edge)for(unsigned i=0;i<actors.size();++i)As_Techno(actors[i])->Coord=XY_Coord(((outside?11:Map.MapCellX+2)+i%4)*ICON_LEPTON_W+ICON_LEPTON_W/2,Coord_Y(origin)+(i/4)*ICON_LEPTON_W);
 root->Assign_Target(TARGET_NONE);root->Coord=origin;root->PrimaryFacing.Set_Current(DIR_W);root->PrimaryFacing.Set_Desired(DIR_W);root->Arm_Fuse(origin,origin,edge?255:1,0);TickBullets();Check("edge or lost-target expiry creates exactly one continuation inside ten cells",Bullets.Count()==(outside?0:1));if(outside)return;
 Checkpoint(dir);for(int hop=1;hop<=10;++hop){ImpactGeneration(false);Check("edge or lost-target chain continues singly up to ten",Bullets.Count()==(hop<10?1:0));if(hop==5)Checkpoint(dir);}Check("ten missed ricochets stop without damage",Bullets.Count()==0&&Loss()==0);
}
static void Legacy(const std::string& dir,int version,int priorHops){
 Reset(dir);auto sam=Source(true);Actors();sam->TechnoClass::Fire_At(actors[0],0);for(int i=0;i<priorHops;++i)ImpactGeneration(false);if(version!=2)Bullets.Ptr(0)->Strength=25;Check("save migration fixture",Save_Game((dir+"/legacy.sav").c_str(),"OLD SAM CHAIN"));
 {std::ifstream in(dir+"/legacy.sav",std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(in)),{});auto pos=bytes.rfind("CNCCHN03");Check("find ten-hop versioned chain footer",pos!=std::string::npos);if(pos==std::string::npos)return;if(version==0)bytes.resize(pos);else{std::uint32_t record[8];std::memcpy(record,bytes.data()+pos+12,sizeof(record));record[1]=2-priorHops;if(version==1)record[2]=25;bytes[pos+7]=char('0'+version);bytes.resize(pos+12+sizeof(record));std::memcpy(&bytes[pos+12],record,sizeof(record));}std::ofstream out(dir+"/legacy.sav",std::ios::binary|std::ios::trunc);out.write(bytes.data(),bytes.size());}
 Check("load old or missing chain extension",Load_Game((dir+"/legacy.sav").c_str()));Check("old in-flight missile keeps stock power",Bullets.Count()==1&&Bullets.Ptr(0)->Strength==50);
 int remaining=version==0?10:10-priorHops;for(int hop=0;hop<=remaining;++hop){ImpactGeneration(true);Check("legacy migration preserves used hops and never branches",Bullets.Count()==(hop<remaining?1:0));if(hop==2)Checkpoint(dir);}Check("legacy chain has correct remaining hit count",Loss()==(remaining+1)*50&&Bullets.Count()==0);
}
static void Scope(const std::string& dir){Reset(dir);auto sam=Source(false);Actors(false,1);auto shot=static_cast<TechnoClass*>(sam)->Fire_At(actors[0],0);Check("enemy retains original power and no branching",shot&&shot->Strength==50&&!Campaign_SAM_Chain_Impact(shot,As_Techno(actors[0])));}
static void LargePool(const std::string& dir){
 Reset(dir);auto sam=Source(true);Check("capacity increased from fifty to sixty-five thousand",BULLET_MAX==65536);BulletClass* last=nullptr;bool allocated=true;
 for(int i=0;i<BULLET_MAX;++i){auto b=new BulletClass(BULLET_SAM);if(!b){allocated=false;break;}b->Strength=50;b->Payback=sam;Campaign_SAM_Chain_Start(b);last=b;}
 Check("all 65536 live projectile slots allocate",allocated&&Bullets.Count()==65536);if(!allocated||!last)return;auto id=last->As_Target();Check("high projectile ID round-trips without truncation",Target_Value(id)==65535&&As_Bullet(id)==last);auto overflow=new BulletClass(BULLET_SAM);Check("capacity boundary returns null safely",overflow==nullptr);if(overflow)delete overflow;
 Check("save all 65536 live projectiles and chain records",Save_Game((dir+"/large-pool.sav").c_str(),"65536 LIVE PROJECTILES"));Check("reload all 65536 projectiles",Load_Game((dir+"/large-pool.sav").c_str()));Check("high IDs and damage survive large-pool reload",Bullets.Count()==65536&&As_Bullet(id)&&As_Bullet(id)->Strength==50);
 Reset(dir);Actors();sam=Source(true);for(int i=0;i<72;++i)sam->TechnoClass::Fire_At(actors[0],0);Check("stress starts seventy-two roots beyond original pool cap",Bullets.Count()==72);for(int hop=1;hop<=11;++hop){ImpactGeneration(false);Check("large volley stays at seventy-two single chains until ten hops complete",Bullets.Count()==(hop<=10?72:0));if(hop==5)Checkpoint(dir);}Check("large volley stops without multiplication or splash",Bullets.Count()==0&&Loss()==0);
}
bool Generate_Mission_Starts(const char* directory){std::string dir(directory);for(bool miss:{false,true})for(bool bonus:{false,true})for(int checkpoint:{0,1,5,10})ForcedChain(dir,miss,bonus,checkpoint);SparseChain(dir,1);SparseChain(dir,2);NativeChain(dir,false,false);NativeChain(dir,true,false);NativeChain(dir,false,true);for(bool outside:{false,true})for(bool miss:{false,true})Boundary(dir,outside,miss);MixedTargets(dir);Exclusions(dir);for(bool edge:{false,true})for(bool bonus:{false,true})EdgeAndLostTarget(dir,edge,bonus);EdgeAndLostTarget(dir,true,false,true);Legacy(dir,0,0);for(int version:{1,2})for(int hops:{0,1,2})Legacy(dir,version,hops);Scope(dir);LargePool(dir);std::printf("TOTAL FAILURES %d\n",failures);return failures==0;}
