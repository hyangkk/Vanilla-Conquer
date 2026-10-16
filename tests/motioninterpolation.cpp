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
    TickSample<unsigned> sample;
    sample.Begin(100); sample.End(132);
    const double halfway=Position(sample.previous,sample.current,.5);
    sample.Begin(132); // Main_Loop renders input before the next Logic.AI call.
    Check(Position(sample.previous,sample.current,.5)==halfway,
          "Beginning a simulation tick must not replace the published interpolation endpoints");
    sample.End(164);
    Check(sample.previous==132 && sample.current==164,
          "Completed logic must publish both new endpoints together");
    Timeline timeline;
    timeline.Record(0,0,0); timeline.Record(.050,10,0); timeline.Record(.133,20,0); timeline.Record(.183,30,0);
    double prior=0;
    for (int frame=0;frame<22;++frame) {
        const auto point=Sample_Timeline(timeline.Points(),frame/120.0);
        Check(point.x>=prior && point.x-prior<1.7, "Uneven logic intervals must not produce position jumps or reversals");
        prior=point.x;
    }
    Check(Sample_Timeline(timeline.Points(),10).x==30, "Buffered timeline must not extrapolate a stalled game");
    const std::vector<Point> short_stop={{0,0,0},{.067,2,0},{.134,4,0},{.201,4,0},{.268,6,0},{.335,8,0}};
    double smoothed=Smooth_Timeline(short_stop,.14,.067).x;
    for (int i=1;i<=7;++i) {
        double next=Smooth_Timeline(short_stop,.14+i/120.0,.067).x;
        Check(next>smoothed, "A one-tick movement plateau must not freeze the display trajectory");
        Check(next>=0 && next<=8, "Display smoothing must remain inside the recorded path bounds");
        smoothed=next;
    }
    Check(Smooth_Timeline(short_stop,10,.067).x==8, "Stopped units must settle at their exact authoritative destination");
    std::puts("motion interpolation: all checks passed");
}
