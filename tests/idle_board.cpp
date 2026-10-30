#include <cassert>
#include "../src/core/IdleBoard.h"
#include "../src/core/ScreenSleep.h"
#include "../src/core/IdleBacklight.h"
int main(){
    using namespace spark;
    IdleBoard b;
    assert(!b.active() && !b.motionReady());
    b.update(100000,false,true);assert(!b.motionReady()); // Inactive board never arms motion.
    // Fn+L: saved brightness for 2 minutes, quarter until 5, then dark and motion-ready.
    static_assert(IdleBoardDimMs==120000 && IdleBoardOffMs==300000,"final idle defaults");
    b.enter(1000);
    b.update(120999,false,true);assert(b.brightness(120999,180)==180 && !b.motionReady());
    b.update(121000,false,true);assert(b.brightness(121000,180)==45 && !b.motionReady());
    b.update(300999,false,true);assert(!b.motionReady());
    b.update(301000,false,true);assert(b.motionReady() && b.brightness(301000,180)==0);
    // Motion or input returns to the board itself, never to an app.
    b.show(310000);assert(b.active() && !b.motionReady() && b.brightness(310000,180)==180);
    b.update(609999,false,true);assert(!b.motionReady());
    b.update(610000,false,true);assert(b.motionReady());
    // Fn+Esc manual sleep is never motion-ready, however long it lasts.
    for(uint32_t t=610000;t<4000000;t+=250){b.update(t,true,true);assert(!b.motionReady());}
    assert(b.brightness(4000000,180,true)==0);
    b.show(4000000);b.update(4000000,false,true);assert(!b.motionReady() && b.brightness(4000000,180)==180);
    // Without a working IMU the board stays dimmed, never dark.
    b.update(9000000,false,false);assert(!b.motionReady() && b.brightness(9000000,180)==45);
    b.leave();assert(!b.active() && !b.motionReady());
    b.show(9000001);assert(!b.active());
    // Timer wrap.
    b.enter(UINT32_MAX-10000);b.update(289998,false,true);assert(!b.motionReady());
    b.update(289999,false,true);assert(b.motionReady());
    b.leave();
    // A reminder relights the board; only a dark board reports that the panel needs waking.
    assert(!b.remind(0));b.enter(0);assert(!b.remind(1000) && b.brightness(1000,180)==180);
    b.update(301000,false,true);assert(b.motionReady());
    assert(b.remind(301000) && !b.motionReady() && b.brightness(301000,180)==180);
    b.update(301000,false,true);assert(!b.motionReady());
    b.update(601000,false,true);assert(b.motionReady()); // Same 2/5-minute timers afterwards.
    b.leave();
    IdleBacklight idle;
    // Timeouts are measured only by elapsed millis: nothing depends on battery or USB power.
    idle.activity(UINT32_MAX-1000);assert(!idle.timedOut(298998) && idle.timedOut(298999));
    // Live USB monitor keeps automatic idle awake; Fn+Esc always sleeps.
    assert(IdleBoard::cpuSleepAllowed(true,true) && IdleBoard::cpuSleepAllowed(true,false));
    assert(!IdleBoard::cpuSleepAllowed(false,true) && IdleBoard::cpuSleepAllowed(false,false));
    // The key that lights a dark board is consumed until released; G0 still escapes.
    ScreenSleep s;bool wake=false;
    s.filter(false,false,false,false,wake);assert(!s.quarantined());
    s.quarantineWake();assert(s.quarantined() && !s.sleeping());
    assert(s.filter(false,true,false,true,wake) && !wake);
    assert(s.filter(false,false,false,false,wake));
    assert(!s.filter(false,true,false,true,wake));
    s.quarantineWake();assert(s.filter(false,false,false,true,wake)); // Stuck key, G0 released.
    assert(!s.filter(false,false,true,true,wake,true) && !s.quarantined()); // Fresh G0 press.
}
