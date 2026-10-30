#pragma once
#include "Screen.h"
#include "../core/ClockFormat.h"
#include "../core/ReminderEngine.h"
namespace spark {
inline void drawReminderNotice(const ReminderEngine& engine,bool saveFailed) {
    frame("REMINDER"); text(6,5,"REMINDER",2,theme::Attention);
    const char* body=engine.record.notice.text;
    for(unsigned row=0;row<4 && *body;++row) {
        char line[20]{}; size_t count=strlen(body); if(count>19) count=19;
        memcpy(line,body,count); body+=count; text(6,32+row*16,line,2,theme::Attention);
    }
    if(saveFailed) text(6,101,"Reminder state save failed",1,theme::Error);
    else if(engine.waitingForTime) text(6,101,"Waiting for time synchronization",1,theme::Attention);
    else {
        char when[24],line[32];formatReminderDateTime(when,sizeof(when),engine.record.notice.due);
        snprintf(line,sizeof(line),"Due %s",when);text(6,101,line,1,theme::Attention);
    }
    text(6,114,"Enter dismiss  1 snooze 10m"); text(6,126,"3 snooze 30m  Fn apps");
}
}
