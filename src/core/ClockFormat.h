#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>
namespace spark {
// Display preference only. Epoch values, time zones and reminder input/storage
// remain unchanged. Accessed on the UI task; no new timer or worker is needed.
inline bool& twelveHourClock(){static bool enabled=false;return enabled;}
template<typename Store> void loadClockFormat(Store& store){twelveHourClock()=store.getUChar("clock12",0)==1;}
template<typename Store> bool saveClockFormat(Store& store,bool enabled){
    if(store.putUChar("clock12",enabled?1:0)!=1)return false;
    twelveHourClock()=enabled;return true;
}
inline void formatClock(char* out,size_t size,int hour,int minute,bool compact=false){
    if(hour<0 || hour>23 || minute<0 || minute>59){snprintf(out,size,"--:--");return;}
    if(!twelveHourClock()){snprintf(out,size,"%02u:%02u",unsigned(hour),unsigned(minute));return;}
    unsigned h=unsigned(hour)%12;if(!h)h=12;
    const char* suffix=compact?(hour<12?"A":"P"):(hour<12?" AM":" PM");
    snprintf(out,size,"%u:%02u%s",h,unsigned(minute),suffix);
}
// Reminder date/time for display: time first, e.g. "7:30 PM 10/23/2026" or "19:30 10/23/2026".
inline void formatReminderDateTime(char* out,size_t size,int64_t epoch){
    time_t value=epoch;struct tm local{};
    if(!localtime_r(&value,&local)){snprintf(out,size,"--:-- --/--/----");return;}
    char clock[9];formatClock(clock,sizeof(clock),local.tm_hour,local.tm_min);
    snprintf(out,size,"%s %02d/%02d/%04d",clock,local.tm_mon+1,local.tm_mday,local.tm_year+1900);
}
inline void formatClockMinute(char* out,size_t size,int minute,bool compact=false){
    if(minute<0 || minute>=1440){snprintf(out,size,"--:--");return;}
    formatClock(out,size,minute/60,minute%60,compact);
}
}
