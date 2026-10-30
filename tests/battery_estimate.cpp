#include <cassert>
#include "../src/core/BatteryEstimate.h"
int main(){
 spark::BatteryEstimate b;
 assert(b.update(-1,0)==-1);assert(b.update(55,0)==55);
 // Plugging in must not turn a voltage jump into an immediate charge jump.
 assert(b.update(83,2000)==55);assert(b.update(83,59999)==55);
 assert(b.update(83,60000)==56);assert(b.update(83,120000)==57);
 assert(b.update(-1,125000)==57);assert(b.update(101,125000)==57);
 assert(b.update(50,130000)==56);assert(b.update(50,132000)==56);
 // A long polling gap cannot produce a catch-up jump.
 assert(b.update(90,900000)==57);
 spark::BatteryEstimate wrap;assert(wrap.update(60,UINT32_MAX-999)==60);
 assert(wrap.update(90,59000)==61);
}
