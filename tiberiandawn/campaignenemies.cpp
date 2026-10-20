// Runtime bookkeeping stays outside serialized game object layouts.
#include "function.h"
#include "campaigndefense.h"
#include "campaignenemies.h"
#include "campaignwaves.h"
#include <cstdint>
#include <cstring>
#include <set>
#include <map>
#include <vector>
namespace {
std::set<TARGET> processed;
struct Companion {
    TARGET extra, target, destination;
    MissionType mission;
};
std::map<TARGET, Companion> companions;
FootClass* Foot(TARGET target)
{
    auto object = As_Techno(target);
    if (!object || !object->IsActive) return nullptr;
    switch(object->What_Am_I()) {
    case RTTI_UNIT: case RTTI_INFANTRY: case RTTI_AIRCRAFT: return static_cast<FootClass*>(object);
    default: return nullptr;
    }
}
void SyncOrders()
{
    for (auto& entry : companions) {
        auto original = Foot(entry.first);
        auto extra = Foot(entry.second.extra);
        if (!original || !extra || original->IsInLimbo || extra->IsInLimbo
            || original->House != extra->House) continue;
        auto& last = entry.second;
        if (original->Mission != last.mission) {
            extra->Assign_Mission(original->Mission); extra->Commence(); last.mission=original->Mission;
        }
        if (original->TarCom != last.target) {
            extra->Assign_Target(original->TarCom); last.target=original->TarCom;
        }
        if (original->NavCom != last.destination) {
            extra->Assign_Destination(original->NavCom); last.destination=original->NavCom;
        }
    }
}
const char magic[8] = {'C','N','C','E','N','M','0','1'};
bool Eligible(FootClass* object)
{
    return object && object->IsActive && !object->IsInLimbo && object->Strength > 0
        && object->House && object->House != PlayerPtr && !object->House->IsHuman
        && object->House->Class->House != HOUSE_NEUTRAL
        && !PlayerPtr->Is_Ally(object->House) && !object->House->Is_Ally(PlayerPtr)
        && Map.In_Radar(Coord_Cell(object->Coord));
}
bool Duplicate(FootClass* original)
{
    FootClass* extra = static_cast<FootClass*>(original->Class_Of().Create_One_Of(original->House));
    if (!extra) return false; // Heap full: retry after losses free space.
    if (original->What_Am_I() == RTTI_AIRCRAFT) {
        static_cast<AircraftClass*>(extra)->Altitude = static_cast<AircraftClass*>(original)->Altitude;
    }
    CELL center = Coord_Cell(original->Coord);
    bool placed = false;
    for (int radius = 0; radius <= 6 && !placed; ++radius) {
        for (int dy = -radius; dy <= radius && !placed; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                if (MAX(ABS(dx), ABS(dy)) != radius) continue;
                int x = Cell_X(center) + dx, y = Cell_Y(center) + dy;
                if (x < 0 || x >= MAP_CELL_W || y < 0 || y >= MAP_CELL_H) continue;
                CELL cell = XY_Cell(x, y);
                if (!Map.In_Radar(cell) || extra->Can_Enter_Cell(cell) != MOVE_OK) continue;
                if (extra->Unlimbo(Cell_Coord(cell), original->PrimaryFacing.Current())) {
                    placed = true;
                    break;
                }
            }
        }
    }
    if (!placed) { delete extra; return false; }
    extra->Strength = original->Strength;
    extra->Ammo = original->Ammo;
    extra->IsALoaner = original->IsALoaner;
    // Never copy mission triggers, transport cargo, radio links or team pointers.
    // Those belong to the original scripted actor; the companion follows its orders.
    extra->Assign_Mission(original->Mission);
    extra->Commence();
    extra->Assign_Target(original->TarCom);
    extra->Assign_Destination(original->NavCom);
    processed.insert(original->As_Target());
    processed.insert(extra->As_Target());
    companions[original->As_Target()] = {extra->As_Target(), original->TarCom, original->NavCom, original->Mission};
    return true;
}
}
void Campaign_Enemies_Reset() { processed.clear(); companions.clear(); Campaign_Waves_Reset(); Campaign_SAM_Health_Reset(); Campaign_SAM_Chain_Reset(); }
void Campaign_Enemies_Mark_Extra(const TechnoClass* object) { processed.insert(object->As_Target()); }
void Campaign_Enemies_Forget(const TechnoClass* object) {
    TARGET id=object->As_Target(); processed.erase(id); companions.erase(id);
    for (auto it=companions.begin(); it!=companions.end();) {
        if (it->second.extra==id) it=companions.erase(it); else ++it;
    }
}
void Campaign_Enemies_Apply()
{
    if (GameToPlay != GAME_NORMAL || !GameActive || !PlayerPtr || !PlayerPtr->IsHuman || ScenarioInit) return;
    SyncOrders();
    std::vector<FootClass*> originals;
    for (int i=0;i<Units.Count();++i) originals.push_back(Units.Ptr(i));
    for (int i=0;i<Infantry.Count();++i) originals.push_back(Infantry.Ptr(i));
    for (int i=0;i<Aircraft.Count();++i) originals.push_back(Aircraft.Ptr(i));
    for (auto object : originals) {
        if (Eligible(object) && !processed.count(object->As_Target())) Duplicate(object);
    }
}
bool Campaign_Enemies_Save(FileClass& file)
{
    std::uint32_t count = processed.size();
    if (file.Write(magic, sizeof(magic)) != sizeof(magic) || file.Write(&count, sizeof(count)) != sizeof(count)) return false;
    for (TARGET target : processed) {
        std::uint32_t id = target;
        if (file.Write(&id, sizeof(id)) != sizeof(id)) return false;
    }
    count = companions.size();
    if (file.Write(&count,sizeof(count)) != sizeof(count)) return false;
    for (auto const& entry : companions) {
        std::uint32_t data[5] = {std::uint32_t(entry.first), std::uint32_t(entry.second.extra),
            std::uint32_t(entry.second.target), std::uint32_t(entry.second.destination), std::uint32_t(entry.second.mission)};
        if (file.Write(data,sizeof(data)) != sizeof(data)) return false;
    }
    return Campaign_Waves_Save(file);
}
bool Campaign_Enemies_Load(FileClass& file)
{
    Campaign_Enemies_Reset();
    char header[sizeof(magic)];
    int bytes = file.Read(header, sizeof(header));
    if (!bytes) return true; // Native old saves have no optional footer.
    if (bytes != sizeof(header) || std::memcmp(header, magic, sizeof(magic))) return false;
    std::uint32_t count;
    if (file.Read(&count, sizeof(count)) != sizeof(count) || count > UNIT_MAX+INFANTRY_MAX+AIRCRAFT_MAX) return false;
    for (std::uint32_t i=0; i<count; ++i) {
        std::uint32_t id;
        if (file.Read(&id, sizeof(id)) != sizeof(id)) return false;
        processed.insert(TARGET(id));
    }
    if (file.Read(&count,sizeof(count)) != sizeof(count) || count > (UNIT_MAX+INFANTRY_MAX+AIRCRAFT_MAX)/2) return false;
    for (std::uint32_t i=0;i<count;++i) {
        std::uint32_t data[5];
        if (file.Read(data,sizeof(data)) != sizeof(data)) return false;
        if (data[4] != std::uint32_t(MISSION_NONE) && data[4] >= MISSION_COUNT) return false;
        companions[TARGET(data[0])] = {TARGET(data[1]),TARGET(data[2]),TARGET(data[3]),MissionType(data[4])};
    }
    return Campaign_Waves_Load(file);
}
