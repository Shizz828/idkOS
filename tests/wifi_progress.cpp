#include <cassert>
#include <initializer_list>
#include "../src/core/WifiProgress.h"
using namespace spark;
int main() {
    WifiProgress progress;
    progress.start(7,0); assert(progress.attempt==7 && progress.phase==WifiPhase::Connecting);
    assert(!progress.update(false,0,19999));
    assert(progress.update(false,0,20000)); assert(progress.phase==WifiPhase::TimedOut);
    progress.start(8,10); assert(progress.update(false,202,11)); assert(progress.phase==WifiPhase::AuthFailed);
    progress.start(9,20); assert(progress.update(false,201,21)); assert(progress.phase==WifiPhase::NotFound);
    progress.start(10,30); assert(!progress.update(false,8,31)); // Ignore deliberate disconnect.
    assert(progress.update(true,202,32)); assert(progress.phase==WifiPhase::Connected);
    assert(progress.update(false,0,33)); assert(progress.phase==WifiPhase::Offline);
    progress.start(11,0xffffff00); assert(!progress.update(false,0,0xffffff01));
    assert(progress.update(false,0,uint32_t(0xffffff00u+20000)));
    assert(progress.phase==WifiPhase::TimedOut);
    for(unsigned char reason:{15,23,204}) {
        progress.start(12,0); progress.update(false,reason,1); assert(progress.phase==WifiPhase::AuthFailed);
    }
}
