#pragma once
#include <stdint.h>
#include <math.h>
namespace spark {
// Four samples/second in motion-ready idle only. No gyro, IRQ or new task.
constexpr uint32_t MotionSampleMs=250;
constexpr uint64_t MotionSleepUs=250000;
class MotionWake {
    float bx=0,by=0,bz=1,fx=0,fy=0,fz=1;
    uint32_t started=0,last=0,candidateAt=0,pulseAt=0;
    unsigned tiltCount=0,pulseCount=0,settleCount=0;
    bool baseline=false,candidate=false,pulse=false;
public:
    void reset(uint32_t now){started=now;last=now;baseline=false;candidate=false;pulse=false;tiltCount=pulseCount=settleCount=0;}
    bool sample(uint32_t now,float x,float y,float z) {
        float norm=sqrtf(x*x+y*y+z*z);
        if(!isfinite(norm)||norm<0.45f||norm>2.0f){candidate=false;tiltCount=0;pulseCount=settleCount=0;pulse=false;return false;}
        if(!baseline){bx=fx=x;by=fy=y;bz=fz=z;baseline=true;last=now;return false;}
        uint32_t gap=uint32_t(now-last);last=now;
        if(gap<70)return false; // Duplicate/too-fast samples cannot satisfy debounce.
        if(gap>MotionSampleMs*3+50){fx=x;fy=y;fz=z;candidate=false;pulse=false;tiltCount=pulseCount=settleCount=0;}
        fx+=0.4f*(x-fx);fy+=0.4f*(y-fy);fz+=0.4f*(z-fz);
        if(uint32_t(now-started)<500)return false; // Screen-off/set-down guard.
        float bn=sqrtf(bx*bx+by*by+bz*bz),fn=sqrtf(fx*fx+fy*fy+fz*fz);
        // >30 degree sustained change from the screen-off orientation.
        bool tilted=(fx*bx+fy*by+fz*bz)<0.8660254f*bn*fn && norm>0.75f && norm<1.25f;
        if(tilted){if(!candidate){candidate=true;candidateAt=now;tiltCount=0;}++tiltCount;}
        else {candidate=false;tiltCount=0;}
        if(candidate && tiltCount>=4 && uint32_t(now-candidateAt)>=300)return true;
        // Straight pickup: sustained acceleration, followed by settling, not a tap.
        if(!pulse && fabsf(norm-1.0f)>0.18f){if(++pulseCount>=3){pulse=true;pulseAt=now;settleCount=0;}}
        else if(!pulse)pulseCount=0;
        if(pulse){
            if(uint32_t(now-pulseAt)>1200){pulse=false;pulseCount=settleCount=0;}
            else if(fabsf(norm-1.0f)<0.06f){if(++settleCount>=3)return true;}
            else settleCount=0;
        }
        return false;
    }
};
}
