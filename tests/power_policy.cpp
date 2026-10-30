#include <cassert>
#include "../src/core/PowerPolicy.h"
int main(){
    using namespace spark;
    assert(BatteryPollMs==10000);
    WifiRetryPolicy retry;assert(retry.waitMs(3)==2000);
    retry.attempted();assert(retry.waitMs(3)==2000);
    retry.attempted();assert(retry.waitMs(3)==2000);
    retry.attempted();assert(retry.waitMs(3)==15000);
    retry.attempted();assert(retry.waitMs(3)==30000);
    retry.attempted();assert(retry.waitMs(3)==60000);
    for(int n=0;n<100;++n)retry.attempted();assert(retry.waitMs(3)==120000);
    retry.reset();assert(retry.waitMs(0)==2000);
    retry.attempted();assert(retry.waitMs(0)==15000);
}
