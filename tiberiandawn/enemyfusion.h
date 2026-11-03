#ifndef TD_ENEMYFUSION_H
#define TD_ENEMYFUSION_H
class ObjectClass;
class TechnoClass;
class FileClass;
namespace EnemyFusion {
int Tier(ObjectClass const* object);
int Multiplier(ObjectClass const* object);
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
