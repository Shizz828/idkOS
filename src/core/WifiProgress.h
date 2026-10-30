#pragma once
#include <stdint.h>
namespace spark {
enum class WifiPhase : uint8_t { Offline, Connecting, Connected, AuthFailed, NotFound, Failed, TimedOut };
inline const char* wifiMessage(WifiPhase phase) {
    switch(phase) {
    case WifiPhase::Connecting:return "Connecting Wi-Fi...";
    case WifiPhase::Connected:return "Wi-Fi connected";
    case WifiPhase::AuthFailed:return "Authentication failed; check password";
    case WifiPhase::NotFound:return "Network not found; check SSID";
    case WifiPhase::Failed:return "Connection failed; retry";
    case WifiPhase::TimedOut:return "Connection timed out; retry";
    default:return "Wi-Fi offline";
    }
}
struct WifiProgress {
    WifiPhase phase=WifiPhase::Offline;
    uint32_t attempt=0, started=0;
    void start(uint32_t id,uint32_t now) { attempt=id; started=now; phase=WifiPhase::Connecting; }
    bool update(bool online,uint8_t reason,uint32_t now) {
        WifiPhase before=phase;
        if(online) phase=WifiPhase::Connected;
        else if(phase==WifiPhase::Connected) phase=WifiPhase::Offline;
        else if(phase==WifiPhase::Connecting) {
            if(reason==201) phase=WifiPhase::NotFound;
            else if(reason==202 || reason==15 || reason==23 || reason==204) phase=WifiPhase::AuthFailed;
            else if(reason && reason!=8) phase=WifiPhase::Failed;
            else if(uint32_t(now-started)>=20000) phase=WifiPhase::TimedOut;
        }
        return phase!=before;
    }
};
}
