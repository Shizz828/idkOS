#include <cassert>
#include <cmath>
#include "../src/core/MotionWake.h"
using namespace spark;
// Feeds 250ms samples from t, returning the first confirmed wake time or 0.
template<class F> uint32_t run(MotionWake& m,uint32_t t,uint32_t until,F f){
    for(;t<until;t+=MotionSampleMs){float x,y,z;f(t,x,y,z);if(m.sample(t,x,y,z))return t;}
    return 0;
}
int main(){
    static_assert(MotionSampleMs==250 && MotionSleepUs==250000,"proven motion cadence");
    MotionWake m;
    // Flat on a desk with light vibration and single taps: never wakes.
    m.reset(0);
    assert(!run(m,0,600000,[](uint32_t t,float& x,float& y,float& z){
        x=0.01f*float(t%7)-0.03f;y=0.01f*float(t%5)-0.02f;z=1.0f+((t/250)%40==0?0.15f:0.004f*float(t%3));}));
    // A 45 degree held tilt wakes after the guard and >=300ms persistence.
    m.reset(0);
    uint32_t woke=run(m,0,10000,[](uint32_t t,float& x,float& y,float& z){
        bool tilted=t>=2000;x=tilted?0.7071f:0;y=0;z=tilted?0.7071f:1;});
    assert(woke>=2300 && woke<=4000);
    // A brief tilt that returns flat before persistence is ignored.
    m.reset(0);
    assert(!run(m,0,10000,[](uint32_t t,float& x,float& y,float& z){
        bool tilted=t==2000;x=tilted?0.7071f:0;y=0;z=tilted?0.7071f:1;}));
    // Straight pickup: sustained acceleration, then settling, without tilting.
    m.reset(0);
    woke=run(m,0,10000,[](uint32_t t,float& x,float& y,float& z){
        x=0;y=0;z=(t>=2000 && t<2750)?1.3f:1.0f;});
    assert(woke>=2750 && woke<=4000);
    // Within the 500ms screen-off guard a tilt cannot count.
    m.reset(0);
    assert(!m.sample(0,0,0,1));assert(!m.sample(250,0.7071f,0,0.7071f));
    // Invalid or missing data never counts as motion.
    m.reset(0);
    assert(!run(m,0,10000,[](uint32_t,float& x,float& y,float& z){x=NAN;y=0;z=1;}));
    assert(!run(m,0,10000,[](uint32_t,float& x,float& y,float& z){x=0;y=0;z=0;}));
    // Wraps across the 32-bit millisecond counter.
    m.reset(UINT32_MAX-5000);
    uint32_t t=UINT32_MAX-5000;bool wrapped=false;
    for(int n=0;n<40 && !wrapped;++n,t+=MotionSampleMs)wrapped=m.sample(t,n>8?0.7071f:0,0,n>8?0.7071f:1);
    assert(wrapped);
}
