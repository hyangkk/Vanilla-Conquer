#ifndef TD_SIEGE_TANK_H
#define TD_SIEGE_TANK_H
class TechnoClass;
class UnitClass;
class BulletClass;
class FileClass;
namespace SiegeTank {
bool Is(TechnoClass const* object);
bool Transitioning(UnitClass const* unit);
bool Toggle(UnitClass* unit);
void AI(UnitClass* unit);
void Forget(UnitClass* unit);
void Reset();
bool Save(FileClass& file);
bool Load(FileClass& file);
bool Impact(BulletClass* bullet, TARGET intended);
}
#endif
