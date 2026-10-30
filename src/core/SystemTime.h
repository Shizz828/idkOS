#pragma once
#include <atomic>
#include <stdint.h>
namespace spark {
// Shared by the SNTP callback and UI task. Reset on each boot; once synchronized,
// the ESP32 system clock remains usable during temporary network loss.
inline std::atomic<bool>& systemTimeSynchronized() {
    static std::atomic<bool> synchronized{false};
    return synchronized;
}
inline bool systemTimeReady(int64_t now) {
    return systemTimeSynchronized().load() && now>=1704067200;
}
// SNTP is started before Wi-Fi has an IP; its failed DNS lookups back off to
// 150 s. Restart it when a connection arrives and no sync has happened yet.
inline bool restartTimeSync(bool wasOnline,bool online,bool synchronized) {
    return online && !wasOnline && !synchronized;
}
}
