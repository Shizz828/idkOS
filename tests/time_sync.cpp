#include <cassert>
#include "../src/core/SystemTime.h"
int main(){
 using spark::restartTimeSync;
 // Only a fresh connection before the first synchronization restarts SNTP.
 assert(restartTimeSync(false,true,false));
 assert(!restartTimeSync(true,true,false));
 assert(!restartTimeSync(false,true,true));
 assert(!restartTimeSync(true,false,false) && !restartTimeSync(false,false,false));
}
