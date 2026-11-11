#pragma once
#include <stdint.h>
namespace spark {
// CalendarIndex is appended so the older indexes (and Fn+1..5, Fn+S) stay the same.
enum AppIndex { CaptureIndex=0,WeatherIndex,CryptoIndex,ConsoleIndex,SettingsIndex,SshIndex,CalendarIndex };
struct AppDescriptor { const char* id; const char* label; char badge; uint16_t color; char shortcut; };
constexpr AppDescriptor Apps[] = {
    {"capture", "CACHE", 'C', 0xFBE0, '1'},
    {"weather", "WEATHER HUB", 'W', 0x07FF, '2'},
    {"crypto", "CRYPTO", '$', 0x07E0, '3'},
    {"console", "LOCAL CONSOLE", 'T', 0x041F, '4'},
    {"settings", "SETTINGS", 'G', 0x8410, '5'},
    {"ssh", "SSH", 'S', 0xF800, 's'},
    {"calendar", "CALENDAR", 'C', 0xFD20, '6'}
};
inline int shortcutApp(char key) {
    for(unsigned i=0;i<sizeof(Apps)/sizeof(Apps[0]);++i)
        if(Apps[i].shortcut==key) return int(i);
    return -1;
}
}
