#pragma once
#include <stdint.h>
#include "IdleBacklight.h"
namespace spark {
constexpr uint32_t IdleBoardDimMs=IdleDimMs,IdleBoardOffMs=IdleOffMs;
// Idle Departure Board: shown at the saved brightness, dimmed after 2 minutes,
// then dark and motion-ready after 5. Only Fn+L enters it; automatic 5-minute
// screen-off is IdleBacklight's. Fn+Esc sleep is never motion-ready.
class IdleBoard {
    uint32_t visibleSince=0;
    bool active_=false,dark=false;
public:
    bool active()const{return active_;}
    void enter(uint32_t now){active_=true;visibleSince=now;dark=false;}
    void leave(){active_=false;dark=false;}
    // Input, motion or a due reminder lights the board and restarts its timers.
    void show(uint32_t now){if(active_){visibleSince=now;dark=false;}}
    // A due reminder lights the board; true when it was dark, so the panel must be woken too.
    bool remind(uint32_t now){bool wasDark=dark;show(now);return wasDark;}
    void update(uint32_t now,bool manualSleep,bool sensorAvailable){
        if(!active_ || manualSleep){dark=false;return;}
        // Without a working IMU the board stays dimmed so it can still be woken.
        dark=sensorAvailable && uint32_t(now-visibleSince)>=IdleBoardOffMs;
    }
    bool motionReady()const{return dark;}
    uint8_t brightness(uint32_t now,uint8_t normal,bool screenOff=false)const {
        if(screenOff || dark)return 0;
        if(uint32_t(now-visibleSince)>=IdleBoardDimMs)return normal?uint8_t(normal/4?normal/4:1):0;
        return normal;
    }
    // Native USB logging stops while peripheral clocks are gated, so a live
    // serial monitor keeps the CPU awake in motion-ready idle. Fn+Esc always sleeps.
    static bool cpuSleepAllowed(bool manualSleep,bool serialMonitor){return manualSleep || !serialMonitor;}
};
}
