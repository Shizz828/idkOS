#include <cassert>
#include <cstring>
#include "../src/core/UiHealth.h"
int main(){
    spark::UiHealth health;unsigned reports=0;
    auto report=[&](const char* phase,uint32_t elapsed){assert(!strcmp(phase,"audio") && elapsed>=5000);++reports;};
    health.poll(10000,report);assert(!reports); // Boot/network initialization isn't a UI stall.
    health.mark(spark::UiPhase::Sleep,10000);health.poll(11000,report);assert(!reports);
    health.mark(spark::UiPhase::Audio,11000);health.poll(15999,report);assert(!reports);
    health.poll(17000,report);assert(reports==1);
    health.poll(18000,report);assert(reports==1);
    health.poll(27000,report);assert(reports==2);
    health.mark(spark::UiPhase::Idle,27001);health.poll(28000,report);assert(reports==2);
}
