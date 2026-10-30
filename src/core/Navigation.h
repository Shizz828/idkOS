#pragma once
#include <stdint.h>
namespace spark {
constexpr unsigned AppCount = 6;
constexpr unsigned RowsPerPage = 2;
constexpr unsigned PageCount = (AppCount + RowsPerPage - 1) / RowsPerPage;
// Home launcher icons: Cache, Weather, Crypto, Terminal, Settings (SSH is Fn+S).
constexpr unsigned HomeAppCount = 5;
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
        int current = selected < HomeAppCount ? int(selected) : (delta > 0 ? -1 : int(HomeAppCount));
        selected = unsigned((current + delta + int(HomeAppCount)) % int(HomeAppCount));
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
