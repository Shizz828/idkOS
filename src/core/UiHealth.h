#pragma once
#include <atomic>
#include <stdint.h>
namespace spark {
enum class UiPhase : uint8_t { Boot,Input,Services,Display,Audio,Sleep,Idle };
inline const char* uiPhaseName(UiPhase phase) {
    switch(phase){case UiPhase::Input:return "input";case UiPhase::Services:return "services";
    case UiPhase::Display:return "display";case UiPhase::Audio:return "audio";
    case UiPhase::Sleep:return "sleep";case UiPhase::Idle:return "idle";default:return "boot";}
}
class UiHealth {
    std::atomic<uint32_t> progress{0};
    std::atomic<UiPhase> phase{UiPhase::Boot};
    uint32_t checked=0,reported=0;
public:
    void mark(UiPhase next,uint32_t now){progress.store(now);phase.store(next);}
    template<typename Report> void poll(uint32_t now,Report report) {
        if(uint32_t(now-checked)<1000)return;
        checked=now;
        auto current=phase.load();uint32_t elapsed=uint32_t(now-progress.load());
        if(current==UiPhase::Boot || elapsed<5000)return;
        if(reported && uint32_t(now-reported)<10000)return;
        reported=now;report(uiPhaseName(current),elapsed);
    }
};
inline UiHealth& uiHealth(){static UiHealth health;return health;}
}
