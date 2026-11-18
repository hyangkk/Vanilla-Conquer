// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
// software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.

// TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed
// in the hope that it will be useful, but with permitted additional restrictions
// under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT
// distributed with this program. You should have received a copy of the
// GNU General Public License along with permitted additional restrictions
// with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection
#ifndef TD_SMOOTHMOTION_H
#define TD_SMOOTHMOTION_H
class ObjectClass;
// Sidecar state: object layouts, save files, gameplay coordinates and RNG are untouched.
void Smooth_Motion_Begin_Tick(int timer_ticks);
void Smooth_Motion_End_Tick();
void Smooth_Motion_Reset();
void Smooth_Motion_Forget(const ObjectClass* object);
void Smooth_Motion_Scene_Begin();
bool Smooth_Motion_Render_Enabled();
void Smooth_Motion_Object_Begin(const ObjectClass* object, unsigned int coordinate);
void Smooth_Motion_Object_End();
#endif
