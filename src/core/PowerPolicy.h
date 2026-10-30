#pragma once
#include <stdint.h>
namespace spark {
constexpr uint32_t BatteryPollMs=10000;
// Immediate first reconnect, a quick pass over saved networks, then exponential
// cooldown. Manual connect/reconnect and waking always reset the backoff.
class WifiRetryPolicy {
    unsigned failures=0;
public:
    void reset(){failures=0;}
    void attempted(){if(failures<8)++failures;}
    uint32_t waitMs(unsigned networks)const {
        unsigned quick=networks?networks:1;
        if(failures<quick)return 2000;
        unsigned shift=failures-quick;
        if(shift>3)shift=3;
        return 15000u<<shift; // up to two minutes, never disables reconnect.
    }
};
struct PowerAudit {
    uint64_t sleepUs=0;
    uint32_t attempts=0,sleeps=0,failures=0,redraws=0,batteryReads=0;
};
}
