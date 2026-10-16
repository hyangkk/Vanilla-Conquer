#include "common/motioninterpolation.h"
#include <cstdio>
#include <cstdlib>
using namespace MotionInterpolation;
static void Check(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}
int main() {
    Check(Fraction(-1, 4.0/60) == 0, "Negative elapsed time must clamp");
    Check(Fraction(1, 4.0/60) == 1, "Pause/stall must not extrapolate");
    Check(Fraction(0, 0) == 1, "Unthrottled simulation must snap");
    Check(Position(100, 200, 0) == 100 && Position(100, 200, 1) == 200, "Tick boundaries must join");
    Check(Position(200, 100, .5) == 150, "Both travel directions must interpolate");
    Check(!Continuous(0,0,257,0,256) && Continuous(100,100,110,90,256), "Teleports must not interpolate");
    int previous=100, current=132;
    double last=100;
    for (int frame=0; frame<=8; ++frame) {
        double x=Position(previous,current,Fraction(frame/120.0,1.0/15));
        Check(x >= last && x <= current, "120Hz motion must remain monotonic and bounded");
        last=x;
    }
    Check(previous==100 && current==132, "Rendering must never mutate authoritative coordinates");
    std::puts("motion interpolation: all checks passed");
}
