#include <cassert>
#include "../src/core/LightSleep.h"
int main(){
    using namespace spark;
    auto timer=enterLightSleep();
    assert(timer.error==ESP_OK && timer.entered && timer.keys==0 && fakePower::entries==1);
    assert(fakePower::interval==1000000 && fakePower::mask==((1ULL<<0)|(1ULL<<11)));
    assert(fakePower::cleanups==2 && fakePower::rtcRestores==2);
    fakePower::cause=ESP_SLEEP_WAKEUP_EXT1;
    for(auto mask:{StandbyKeyboardMask,StandbyHomeMask,StandbyKeyboardMask|StandbyHomeMask}){
        fakePower::wake=mask;auto key=enterLightSleep();assert(key.keys==mask);
    }
    assert(fakePower::entries==4);
    // Pending input wins the race just before entry. Setup failures never sleep.
    fakePower::pending=true;assert(!enterLightSleep().entered);assert(fakePower::entries==4);
    fakePower::pending=false;fakePower::error=7;
    auto failed=enterLightSleep();assert(failed.error==7 && !failed.entered && failed.keys==0 && fakePower::entries==4);
    assert(fakePower::cleanups==12 && fakePower::rtcRestores==12);
    // Motion-ready idle requests a shorter timer; Fn+Esc keeps the 1s default.
    fakePower::error=0;assert(enterLightSleep(250000).entered && fakePower::interval==250000);
    assert(enterLightSleep().entered && fakePower::interval==1000000);
}
