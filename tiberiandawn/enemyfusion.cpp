// Per-object evolution lives outside native object/save layouts.
#include "function.h"
#include "enemyfusion.h"
#include "campaignenemies.h"
#include "campaigndefense.h"
#include "siegetank.h"
#include "common/renderlayers.h"
#include <map>
#include <vector>
#include <set>
#include <cstdint>
#include <cstring>
namespace EnemyFusion {
namespace {
std::map<TARGET, unsigned> tiers;
const char magic[8]={'C','N','C','F','U','S','0','1'};
Counts lastCounts;
std::vector<unsigned char> counterUnderlay;
int underlayX=0,underlayY=0,underlayWidth=0,underlayHeight=0;
std::uintptr_t underlayCanvas=0;
const unsigned maxTier=5;
const unsigned legacyMaxTier=15;
const unsigned fusionCount=5;
const int counterInset=6,counterWidth=130,counterHeight=24+(maxTier+1)*18;
bool Counter_Visible(){
    return GameActive && !ScenarioInit && GameToPlay==GAME_NORMAL && PlayerPtr && PlayerPtr->IsHuman && !Debug_Map
        && LogicPage && WindowList[WINDOW_TACTICAL][WINDOWWIDTH]>=counterWidth+2*counterInset
        && WindowList[WINDOW_TACTICAL][WINDOWHEIGHT]>=counterHeight+2*counterInset
        && WindowList[WINDOW_TACTICAL][WINDOWX]>=0 && WindowList[WINDOW_TACTICAL][WINDOWY]>=0
        && WindowList[WINDOW_TACTICAL][WINDOWX]+counterInset+counterWidth<=LogicPage->Get_Width()
        && WindowList[WINDOW_TACTICAL][WINDOWY]+counterInset+counterHeight<=LogicPage->Get_Height();
}
bool Mobile(ObjectClass const* o) {
    return o && (o->What_Am_I()==RTTI_UNIT || o->What_Am_I()==RTTI_INFANTRY || o->What_Am_I()==RTTI_AIRCRAFT);
}
bool Ready(FootClass* o) {
    return Mobile(o) && o->IsActive && !o->IsInLimbo && o->Strength>0 && Campaign_Enemy_House(o->House)
        && Map.In_Radar(Coord_Cell(o->Coord)) && !o->IsTethered && !o->In_Radio_Contact()
        && !o->Is_Something_Attached() && !o->Tiberium_Load()
        && (o->What_Am_I()!=RTTI_UNIT || !SiegeTank::Transitioning(static_cast<UnitClass*>(o)));
}
bool Same(FootClass* a,FootClass* b) {
    return a->House==b->House && &a->Class_Of()==&b->Class_Of() && Tier(a)==Tier(b)
        && (a->What_Am_I()!=RTTI_AIRCRAFT ||
            (static_cast<AircraftClass*>(a)->Altitude>0)==(static_cast<AircraftClass*>(b)->Altitude>0));
}
bool ValidID(TARGET id) {
    return (Is_Target_Unit(id) && Target_Value(id)<UNIT_MAX)
        || (Is_Target_Infantry(id) && Target_Value(id)<INFANTRY_MAX)
        || (Is_Target_Aircraft(id) && Target_Value(id)<AIRCRAFT_MAX);
}
}
int Tier(ObjectClass const* object) {
    if(!Mobile(object))return 0;
    auto it=tiers.find(object->As_Target());return it==tiers.end()?0:it->second;
}
int Multiplier(ObjectClass const* object) {return 1<<Tier(object);}
int Visual_Scale(ObjectClass const* object) {
    std::int64_t numerator=256,denominator=1;
    for(int i=0,tier=MIN(int(maxTier),Tier(object));i<tier;++i){numerator*=13;denominator*=10;}
    return int((numerator+denominator/2)/denominator);
}
int Visual_Pixels(ObjectClass const* object,int pixels) {
    std::int64_t scaled=std::int64_t(pixels)*Visual_Scale(object);
    return int(scaled>=0?(scaled+128)/256:-((-scaled+128)/256));
}
Counts Count_Units() {
    Counts result{};
    auto count=[&](TechnoClass const* o) {
        if(o->IsActive && !o->IsInLimbo && o->Strength>0 && Campaign_Enemy_House(o->House)
            && Map.In_Radar(Coord_Cell(o->Coord)))++result[Tier(o)];
    };
    for(int i=0;i<Units.Count();++i)count(Units.Ptr(i));
    for(int i=0;i<Infantry.Count();++i)count(Infantry.Ptr(i));
    for(int i=0;i<Aircraft.Count();++i)count(Aircraft.Ptr(i));
    return result;
}
void Restore_Counts() {
#ifndef REMASTER_BUILD
    if(LogicPage!=&HidPage || counterUnderlay.empty())return;
    if(HidPage.Get_Width()==underlayWidth && HidPage.Get_Height()==underlayHeight && HidPage.Lock()){
        auto pixels=reinterpret_cast<unsigned char*>(HidPage.Get_Offset());
        const int pitch=HidPage.Get_Width()+HidPage.Get_XAdd()+HidPage.Get_Pitch();
        if(reinterpret_cast<std::uintptr_t>(pixels)==underlayCanvas)
            for(int y=0;y<counterHeight;++y)
                std::memcpy(pixels+(underlayY+y)*pitch+underlayX,counterUnderlay.data()+y*counterWidth,counterWidth);
        HidPage.Unlock();
    }
    counterUnderlay.clear();
#endif
}
void Draw_Counts() {
#ifndef REMASTER_BUILD
    Restore_Counts();
    if(!Counter_Visible())return;
    auto counts=Count_Units();
    const int x=WindowList[WINDOW_TACTICAL][WINDOWX]+counterInset;
    const int y=WindowList[WINDOW_TACTICAL][WINDOWY]+counterInset;
    const int width=counterWidth,height=counterHeight;
    if(!LogicPage->Lock())return;
    if(LogicPage==&HidPage){
        auto pixels=reinterpret_cast<const unsigned char*>(HidPage.Get_Offset());
        const int pitch=HidPage.Get_Width()+HidPage.Get_XAdd()+HidPage.Get_Pitch();
        counterUnderlay.resize(width*height);
        for(int row=0;row<height;++row)
            std::memcpy(counterUnderlay.data()+row*width,pixels+(y+row)*pitch+x,width);
        underlayX=x;underlayY=y;underlayWidth=HidPage.Get_Width();underlayHeight=HidPage.Get_Height();
        underlayCanvas=reinterpret_cast<std::uintptr_t>(pixels);
    }
    LogicPage->Fill_Rect(x,y,x+width-1,y+height-1,BLACK);
    LogicPage->Draw_Rect(x,y,x+width-1,y+height-1,GREY);
    auto flags=TPF_6POINT|TPF_NOSHADOW;
    Fancy_Text_Print("ENEMY UNITS",x+6,y+4,LTGREY,TBLACK,flags);
    int row=0;
    for(unsigned tier=0;tier<=maxTier;++tier){
        int top=y+21+row++*18;
        if(!tier)Fancy_Text_Print("NORMAL",x+6,top,WHITE,TBLACK,flags);
        else Fancy_Text_Print("LEVEL %d",x+6,top,WHITE,TBLACK,flags,int(tier));
        Fancy_Text_Print("%d",x+width-7,top,YELLOW,TBLACK,flags|TPF_RIGHT,counts[tier]);
    }
    // Hardware fills bypass rasterizer coverage. Keep the entire panel opaque
    // even where its black pixels equal the previous world pixels.
    const int pitch=LogicPage->Get_Width()+LogicPage->Get_XAdd()+LogicPage->Get_Pitch();
    RenderLayers::Mark(reinterpret_cast<const unsigned char*>(LogicPage->Get_Offset())+y*pitch+x,width,height,pitch);
    LogicPage->Unlock();
#endif
}
int Health(ObjectClass const* object,int base) {return int(std::min(std::int64_t(32767),std::int64_t(base)*Multiplier(object)));}
int Firepower(TechnoClass const* object,int base) {return Health(object,base);}
void Forget(ObjectClass const* object) {if(Mobile(object))tiers.erase(object->As_Target());}
void Reset(){counterUnderlay.clear();tiers.clear();lastCounts.fill(-1);}
void Redraw(){
    auto counts=Count_Units();
    if(!tiers.empty() || counts!=lastCounts)Map.Flag_To_Redraw(true);
    lastCounts=counts;
}
void AI() {
    if(!GameActive || ScenarioInit || GameToPlay!=GAME_NORMAL || !PlayerPtr || !PlayerPtr->IsHuman)return;
    std::vector<TARGET> objects;
    for(int i=0;i<Units.Count();++i)objects.push_back(Units.Ptr(i)->As_Target());
    for(int i=0;i<Infantry.Count();++i)objects.push_back(Infantry.Ptr(i)->As_Target());
    for(int i=0;i<Aircraft.Count();++i)objects.push_back(Aircraft.Ptr(i)->As_Target());
    std::set<TARGET> consumed;
    for(auto anchorID:objects) {
        if(consumed.count(anchorID))continue;
        auto anchor=static_cast<FootClass*>(As_Techno(anchorID));
        if(!anchor || !Ready(anchor) || Tier(anchor)>=int(maxTier))continue;
        std::vector<FootClass*> group{anchor};
        for(auto otherID:objects) {
            if(consumed.count(otherID))continue;
            auto other=static_cast<FootClass*>(As_Techno(otherID));
            if(!other || other==anchor || !Ready(other) || !Same(anchor,other))continue;
            // Mission-linked actors cannot be discarded; an anchor keeps its own trigger and orders.
            if(other->Trigger || ::Distance(anchor->Coord,other->Coord)>10*ICON_LEPTON_W)continue;
            group.push_back(other);if(group.size()==fusionCount)break;
        }
        if(group.size()!=fusionCount)continue;
        std::int64_t ratios=0;
        for(auto member:group)ratios+=std::min<std::int64_t>(65536,std::int64_t(member->Strength)*65536/MAX(1,Campaign_Max_Health(member)));
        tiers[anchor->As_Target()]=Tier(anchor)+1;
        anchor->Strength=MAX(1,int(std::int64_t(Campaign_Max_Health(anchor))*ratios/(fusionCount*65536)));
        anchor->Mark(MARK_CHANGE);
        // Native deletion releases occupancy, team/radio references and heap counts without combat deaths or crew.
        for(unsigned i=1;i<group.size();++i) {
            auto member=group[i];TARGET id=member->As_Target();consumed.insert(id);
            member->Stun();Campaign_Enemies_Forget(member);delete member;
        }
        // A surviving anchor may only evolve once per pass; future passes can fuse the next tier.
        consumed.insert(anchor->As_Target());
        Map.Flag_To_Redraw(true);
    }
}
bool Save(FileClass& file) {
    std::uint32_t count=tiers.size();
    if(file.Write(magic,8)!=8 || file.Write(&count,4)!=4)return false;
    for(auto const& e:tiers){std::uint32_t data[2]={std::uint32_t(e.first),e.second};if(file.Write(data,8)!=8)return false;}
    return true;
}
bool Load(FileClass& file) {
    Reset();char header[8];int n=file.Read(header,8);if(!n)return true;
    if(n!=8 || std::memcmp(header,magic,8))return false;
    std::uint32_t count;if(file.Read(&count,4)!=4 || count>UNIT_MAX+INFANTRY_MAX+AIRCRAFT_MAX)return false;
    for(unsigned i=0;i<count;++i){std::uint32_t data[2];
        if(file.Read(data,8)!=8 || !ValidID(data[0]) || !data[1] || data[1]>legacyMaxTier || tiers.count(data[0]))return false;
        // Parsing precedes Decode_All_Pointers: never dereference Class/House here.
        tiers[data[0]]=data[1];
    }
    return true;
}
bool Fixup() {
    for(auto& e:tiers){
        auto object=As_Techno(e.first);
        if(!object || !Mobile(object) || !object->IsActive)return false;
        if(e.second>maxTier){
            // Prior saves support tiers up to 15. Normalize after pointer decoding, preserving health percentage.
            const int oldMaximum=Campaign_Max_Health(object);
            const int oldHealth=object->Strength;
            e.second=maxTier;
            if(oldHealth>0)object->Strength=MAX(1,int((std::int64_t(oldHealth)*Campaign_Max_Health(object)+oldMaximum/2)/oldMaximum));
        }
    }
    return true;
}
}
