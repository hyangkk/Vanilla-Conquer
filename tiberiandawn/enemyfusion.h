#ifndef TD_ENEMYFUSION_H
#define TD_ENEMYFUSION_H
#include <array>
class ObjectClass;
class TechnoClass;
class FileClass;
namespace EnemyFusion {
int Tier(ObjectClass const* object);
int Multiplier(ObjectClass const* object); // Combat stats: 2^tier.
int Visual_Scale(ObjectClass const* object); // 8-bit fractional scale: 256 * 1.5^tier.
int Visual_Pixels(ObjectClass const* object, int pixels);
using Counts = std::array<int, 16>;
Counts Count_Units();
void Draw_Counts();
int Health(ObjectClass const* object, int base);
int Firepower(TechnoClass const* object, int base);
void AI();
void Redraw();
void Reset();
void Forget(ObjectClass const* object);
bool Save(FileClass& file);
bool Load(FileClass& file);
bool Fixup();
}
#endif
