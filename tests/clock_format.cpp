#include <cassert>
#include <cstring>
#include <cstdlib>
#include <map>
#include <string>
#include "../src/core/ClockFormat.h"
struct Store {
 std::map<std::string,uint8_t> values;bool fail=false;
 uint8_t getUChar(const char* key,uint8_t fallback){auto it=values.find(key);return it==values.end()?fallback:it->second;}
 size_t putUChar(const char* key,uint8_t value){if(fail)return 0;values[key]=value;return 1;}
};
int main(){
 using namespace spark;Store store;loadClockFormat(store);assert(!twelveHourClock());
 char out[16];formatClock(out,sizeof(out),0,0);assert(!strcmp(out,"00:00"));
 formatClock(out,sizeof(out),23,59);assert(!strcmp(out,"23:59"));
 assert(saveClockFormat(store,true));assert(twelveHourClock());
 formatClock(out,sizeof(out),0,0);assert(!strcmp(out,"12:00 AM"));
 formatClock(out,sizeof(out),12,0);assert(!strcmp(out,"12:00 PM"));
 formatClock(out,sizeof(out),13,5);assert(!strcmp(out,"1:05 PM"));
 formatClock(out,sizeof(out),23,59,true);assert(!strcmp(out,"11:59P"));
 twelveHourClock()=false;loadClockFormat(store);assert(twelveHourClock()); // simulated restart
 store.fail=true;assert(!saveClockFormat(store,false));assert(twelveHourClock());
 loadClockFormat(store);assert(twelveHourClock());
 store.fail=false;assert(saveClockFormat(store,false));loadClockFormat(store);assert(!twelveHourClock());
 store.values["clock12"]=255;loadClockFormat(store);assert(!twelveHourClock());
 for(bool twelve:{false,true}){
  twelveHourClock()=twelve;
  for(int minute=0;minute<1440;++minute){
   char compact[7],full[9];formatClockMinute(compact,sizeof(compact),minute,true);formatClockMinute(full,sizeof(full),minute);
   assert(strlen(compact)<sizeof(compact) && strlen(full)<sizeof(full));
   if(twelve)assert(compact[strlen(compact)-1]==(minute<720?'A':'P'));
  }
  formatClockMinute(out,sizeof(out),-1);assert(!strcmp(out,"--:--"));
  formatClockMinute(out,sizeof(out),1440);assert(!strcmp(out,"--:--"));
 }
 // Reminder date/time display: time before date, following the 12/24-hour setting.
 setenv("TZ","UTC0",1);tzset();char when[24];
 const int64_t evening=1792783800; // 2026-10-23 19:30 UTC
 twelveHourClock()=true;formatReminderDateTime(when,sizeof(when),evening);assert(!strcmp(when,"7:30 PM 10/23/2026"));
 twelveHourClock()=false;formatReminderDateTime(when,sizeof(when),evening);assert(!strcmp(when,"19:30 10/23/2026"));
 const int64_t morning=1767600300; // 2026-01-05 08:05 UTC
 formatReminderDateTime(when,sizeof(when),morning);assert(!strcmp(when,"08:05 01/05/2026"));
 twelveHourClock()=true;formatReminderDateTime(when,sizeof(when),morning);assert(!strcmp(when,"8:05 AM 01/05/2026"));
 formatReminderDateTime(when,sizeof(when),1767571200);assert(!strcmp(when,"12:00 AM 01/05/2026")); // midnight
 twelveHourClock()=false;
}
