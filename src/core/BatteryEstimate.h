#pragma once
#include <stdint.h>
namespace spark {
// The board's percentage is voltage-derived, not a measured charge counter.
// Bound changes even when charging detection is unavailable on ADC hardware.
class BatteryEstimate {
    int level=-1;
    uint32_t lastChange=0;
public:
    int update(int raw,uint32_t now) {
        if(raw<0 || raw>100)return level;
        if(level<0){level=raw;lastChange=now;return level;}
        uint32_t interval=raw>level?60000:10000;
        if(raw==level){lastChange=now;return level;}
        if(uint32_t(now-lastChange)>=interval){level+=raw>level?1:-1;lastChange=now;}
        return level;
    }
};
}
