#pragma once
#include <atomic>
#include <stdint.h>
namespace spark {
// Generation-tagged handshake: rapid wake/sleep must not reuse an old Wi-Fi-off
// acknowledgement while the worker is restarting the radio.
class NetworkStandby {
    std::atomic<uint32_t> requested{0},acknowledged{0};
public:
    void request(bool paused) {
        uint32_t old=requested.load();
        if(bool(old&1)!=paused)requested.store(((old+2)&~uint32_t(1)) | uint32_t(paused));
    }
    uint32_t ticket()const{return requested.load();}
    static bool pausing(uint32_t ticket){return ticket&1;}
    void acknowledge(uint32_t ticket){acknowledged.store(ticket);}
    bool ready()const {
        uint32_t current=requested.load();
        return pausing(current) && acknowledged.load()==current;
    }
};
}
