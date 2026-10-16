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
#include <vector>

namespace MotionInterpolation {
template<class Coordinate> struct TickSample {
    Coordinate previous = 0, current = 0, pending = 0;
    bool published = false;
    void Begin(Coordinate coordinate) {
        pending = coordinate;
        if (!published) previous = current = coordinate;
    }
    void End(Coordinate coordinate) {
        previous = pending;
        current = coordinate;
        published = true;
    }
};
inline double Fraction(double elapsed, double duration)
{
    return duration > 0.0 ? std::max(0.0, std::min(1.0, elapsed / duration)) : 1.0;
}
inline double Position(int previous, int current, double fraction)
{
    return previous + (current - previous) * std::max(0.0, std::min(1.0, fraction));
}
struct Point { double time, x, y; };
class Timeline {
    std::vector<Point> points;
public:
    void Clear() { points.clear(); }
    void Record(double time, double x, double y) {
        if (!points.empty() && time<=points.back().time) return;
        points.push_back({time,x,y});
        if (points.size()>8) points.erase(points.begin());
    }
    const std::vector<Point>& Points() const { return points; }
};
inline Point Sample_Timeline(const std::vector<Point>& points, double time) {
    if (points.empty()) return {time,0,0};
    if (time<=points.front().time) return points.front();
    for (size_t i=1;i<points.size();++i) {
        if (time<=points[i].time) {
            const Point& a=points[i-1]; const Point& b=points[i];
            const double t=Fraction(time-a.time,b.time-a.time);
            return {time,a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t};
        }
    }
    return points.back(); // Stalls pause visually; never extrapolate through blockers.
}

inline bool Continuous(int px, int py, int cx, int cy, int max_step)
{
    return std::abs(cx - px) <= max_step && std::abs(cy - py) <= max_step;
}
}
#endif
