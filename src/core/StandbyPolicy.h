#pragma once
#include <stdint.h>
namespace spark {
// At most one second between reminder checks. No timers replay the boot path.
constexpr uint64_t StandbyIntervalUs=1000000;
inline bool canEnterStandby(bool armed,bool inputPending,bool networkReady,
                            bool storageIdle,bool sshBusy,bool toneActive) {
    return armed && !inputPending && networkReady && storageIdle && !sshBusy && !toneActive;
}
}
