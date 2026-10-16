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
// Display-only interpolation. Never feed these values back into simulation state.
#ifndef MOTIONINTERPOLATION_H
#define MOTIONINTERPOLATION_H
#include <algorithm>
#include <cmath>

namespace MotionInterpolation {
inline double Fraction(double elapsed, double duration)
{
    return duration > 0.0 ? std::max(0.0, std::min(1.0, elapsed / duration)) : 1.0;
}
inline double Position(int previous, int current, double fraction)
{
    return previous + (current - previous) * std::max(0.0, std::min(1.0, fraction));
}
inline bool Continuous(int px, int py, int cx, int cy, int max_step)
{
    return std::abs(cx - px) <= max_step && std::abs(cy - py) <= max_step;
}
}
#endif
