#pragma once
#include "Screen.h"
#include "../core/ClockFormat.h"
#include <time.h>
namespace spark {
struct StatusState { bool online=false; int battery=-1,clockMinute=-1; };
inline int statusClockMinute(int64_t now,bool synchronized) {
    if(!synchronized || now<1704067200)return -1;
    time_t epoch=now;struct tm local{};localtime_r(&epoch,&local);return local.tm_hour*60+local.tm_min;
}
inline void drawStatus(const StatusState& state,bool showClock=true) {
    auto& d=screen(); d.fillRect(142,0,98,12,theme::Background);
    char clock[7];formatClockMinute(clock,sizeof(clock),state.clockMinute,true);
    if(showClock)text(142,2,clock,1,theme::Content);
    uint16_t wifi=state.online?theme::Success:theme::Error;
    d.drawLine(179,3,190,3,wifi); d.drawLine(181,5,188,5,wifi);
    d.drawLine(183,7,186,7,wifi); d.fillCircle(184,9,1,wifi);
    if(!state.online) d.drawLine(179,10,190,1,wifi);
    char percent[8];
    if(state.battery>=0 && state.battery<=100) snprintf(percent,sizeof(percent),"%d%%",state.battery);
    else snprintf(percent,sizeof(percent),"--%%");
    text(200,2,percent,1,state.battery>=0 && state.battery<=15?theme::Error:theme::Primary);
}
}
