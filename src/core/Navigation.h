#pragma once
#include <stdint.h>
namespace spark {
constexpr unsigned AppCount = 7;
constexpr unsigned RowsPerPage = 2;
constexpr unsigned PageCount = (AppCount + RowsPerPage - 1) / RowsPerPage;
// Home launcher icons, left to right: Cache, Weather, Crypto, Terminal, Calendar,
// Settings (Settings stays last). Entries are app indexes (see AppRegistry); SSH is
// Fn+S and has no icon. Calendar is Fn+6 so Fn+1..5 keep their V1 meaning.
constexpr unsigned HomeAppCount = 6;
constexpr unsigned HomeOrder[HomeAppCount] = {0, 1, 2, 3, 6, 4};
inline int homePosition(unsigned app) {
    for (unsigned i = 0; i < HomeAppCount; ++i) if (HomeOrder[i] == app) return int(i);
    return -1;
}
struct Navigation {
    unsigned selected = 0;
    int active = -1;
    void move(int delta) { selected = (selected + AppCount + delta) % AppCount; }
    void page(int delta) {
        unsigned p = selected / RowsPerPage;
        p = (p + PageCount + delta) % PageCount;
        selected = p * RowsPerPage;
    }
    // Left/Right on Home. After Fn+S (SSH, not an icon) it starts from either end.
    void moveHome(int delta) {
        int current = homePosition(selected);
        if (current < 0) current = delta > 0 ? -1 : int(HomeAppCount);
        selected = HomeOrder[(current + delta + int(HomeAppCount)) % int(HomeAppCount)];
    }
    void open() { active = selected; }
    void home() { active = -1; }
};
// Repeating keys: first press immediately, then after 400ms, every 80ms.
struct RepeatKey {
    bool held = false;
    uint32_t next = 0;
    bool update(bool down, uint32_t now) {
        if (!down) { held = false; return false; }
        if (!held) { held = true; next = now + 400; return true; }
        if (int32_t(now - next) >= 0) { next = now + 80; return true; }
        return false;
    }
};
}
