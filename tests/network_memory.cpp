#include <cassert>
#include "../src/core/NetworkMemory.h"
using namespace spark;
int main(){
 NetworkMemoryGate gate;
 {HttpMemoryLease first(gate);assert(first && gate.httpActive());HttpMemoryLease overlap(gate);assert(!overlap);gate.requestSsh();assert(!gate.trySsh());}
 assert(!gate.httpActive());assert(!gate.tryHttp());assert(gate.trySsh());assert(!gate.tryHttp());gate.releaseSsh();
 {HttpMemoryLease resumed(gate);assert(resumed);}
 gate.requestSsh();gate.cancelSsh();assert(gate.tryHttp());gate.releaseHttp();
 for(unsigned i=0;i<1000;++i){gate.requestSsh();assert(gate.trySsh());gate.releaseSsh();assert(gate.tryHttp());gate.releaseHttp();}
}
