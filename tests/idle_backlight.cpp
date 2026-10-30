#include <cassert>
#include "../src/core/IdleBacklight.h"
#include "../src/core/ScreenSleep.h"
#include "../src/core/IdleBoard.h"
int main(){
    using namespace spark;
    IdleBacklight idle;idle.activity(500);
    assert(idle.brightness(120499,180)==180);
    assert(idle.brightness(120500,180)==45);
    assert(idle.brightness(300499,180)==45);
    assert(idle.brightness(300500,180)==0);
    idle.activity(300501);assert(idle.brightness(300501,180)==180);
    assert(idle.brightness(420500,180)==180);
    assert(idle.brightness(420501,180)==45);
    // Settings changes use the current value; dimming never changes a preference.
    assert(idle.brightness(420501,80)==20);
    idle.activity(420502);assert(idle.brightness(420502,80)==80);
    assert(idle.brightness(420502,80,true)==0);
    idle.activity(1000000);assert(idle.brightness(1000000,80)==80);
    idle.activity(UINT32_MAX-59999);
    assert(idle.brightness(59999,100)==100);
    assert(idle.brightness(60000,100)==25);
    assert(idle.brightness(240000,100)==0);
    idle.activity(0);assert(idle.brightness(120000,1)==1);
    assert(idle.brightness(120000,0)==0);
    // Automatic backlight-off never enters the manual sleep state or consumes
    // the next app input. Manual Fn+Esc ignores keys and consumes its G0 wake.
    ScreenSleep manual;bool wake=false;
    manual.filter(false,false,false,false,wake);
    assert(idle.brightness(300000,80)==0 && !manual.sleeping());
    assert(!manual.filter(false,true,false,true,wake));
    idle.activity(300001);assert(idle.brightness(300001,80)==80);
    assert(manual.filter(true,true,false,true,wake) && manual.sleeping());
    manual.filter(false,false,false,false,wake);
    assert(manual.filter(false,true,false,true,wake) && !wake && manual.sleeping());
    assert(manual.filter(false,false,true,true,wake,true) && wake && !manual.sleeping());
    // Automatic screen-off: one inactivity manager for every normal app screen.
    IdleBacklight a;a.activity(0);
    // 1. Activity resets the inactivity timer.
    a.activity(250000);assert(!a.update(300000,false) && a.brightness(300000,180)==180);
    assert(!a.update(549999,false) && !a.screenOff());
    // 2. Two minutes dims, without turning the screen off.
    assert(a.brightness(369999,180)==180 && a.brightness(370000,180)==45 && !a.screenOff());
    // 3. Five minutes turns the screen fully off, from any app; no IMU is involved.
    assert(a.update(550000,false) && a.screenOff() && a.brightness(550000,180)==0);
    assert(a.update(9000000,false) && a.brightness(9000000,180)==0); // Stays off until woken.
    // 4. Wake resumes the previous screen: no idle board, no Home, no PIN.
    IdleBoard board;
    assert(a.wake(9000000) && !a.screenOff() && a.brightness(9000000,180)==180);
    assert(!board.active() && !a.wake(9000001)); // Only a real wake reports one.
    ScreenSleep resume;resume.filter(false,false,false,false,wake);
    resume.quarantineWake(); // The waking key is consumed...
    assert(resume.filter(false,true,false,true,wake) && !wake && !resume.sleeping());
    assert(resume.filter(false,false,false,false,wake));
    assert(!resume.filter(false,true,false,true,wake)); // ...then keys reach the same app.
    a.activity(9000002);assert(!a.update(9299999,false) && a.update(9300002,false)); // Off again.
    // 5. Fn+L stays separate: it never starts automatic screen-off and keeps its own board.
    a.activity(0);board.enter(0);
    assert(!a.update(400000,board.active()) && !a.screenOff());
    board.update(400000,false,true);assert(board.motionReady() && board.brightness(400000,180)==0);
    board.show(400001);assert(board.active() && !board.motionReady()); // Wakes to the board, not the app.
    board.leave();a.activity(400002);
    // 6. Fn+Esc is unchanged: no automatic screen-off under it, and only G0 wakes.
    ScreenSleep pocket;pocket.filter(false,false,false,false,wake);
    pocket.filter(true,true,false,true,wake);pocket.filter(false,false,false,false,wake);
    assert(pocket.sleeping() && !a.update(9000000,pocket.sleeping()) && !a.screenOff());
    assert(a.brightness(9000000,180,true)==0);
    assert(pocket.filter(false,true,false,true,wake) && !wake && pocket.sleeping());
    assert(pocket.filter(false,false,true,true,wake,true) && wake && !pocket.sleeping());
}
