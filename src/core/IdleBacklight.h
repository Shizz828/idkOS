#pragma once
#include <stdint.h>
namespace spark {
// Inactivity defaults, the same on battery and USB: dim at 2 minutes, then
// automatic screen-off (dark, motion-ready) at 5 minutes on any screen.
constexpr uint32_t IdleDimMs=120000,IdleOffMs=300000;
class IdleBacklight {
    uint32_t lastActivity=0;
    bool off=false;
public:
    // Input, motion or a due reminder restarts the timers and ends an automatic screen-off.
    void activity(uint32_t now){lastActivity=now;off=false;}
    bool timedOut(uint32_t now)const{return uint32_t(now-lastActivity)>=IdleOffMs;}
    // Automatic screen-off keeps the current screen and has no lock. The Fn+L idle
    // board and Fn+Esc sleep have their own rules, so neither starts it.
    bool update(uint32_t now,bool otherIdle){if(!otherIdle && timedOut(now))off=true;return off;}
    bool screenOff()const{return off;}
    // True when an automatic screen-off was ended, so the panel must be woken.
    bool wake(uint32_t now){if(!off)return false;activity(now);return true;}
    uint8_t brightness(uint32_t now,uint8_t normal,bool manualSleep=false)const {
        if(manualSleep || off)return 0;
        const uint32_t idle=uint32_t(now-lastActivity);
        if(idle>=IdleOffMs)return 0;
        // Dim to one quarter of the current user setting, never increase it.
        if(idle>=IdleDimMs)return normal?uint8_t(normal/4?normal/4:1):0;
        return normal;
    }
};
}
