#pragma once
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "../storage/NoteData.h"
namespace spark {
constexpr int64_t ReminderClockMinimum=1704067200;
inline int64_t parseReminderDate(const char* s) {
    int y,m,d,h,min,n=0;
    if(strlen(s)!=16 || sscanf(s,"%4d-%2d-%2d %2d:%2d%n",&y,&m,&d,&h,&min,&n)!=5 || n!=16 ||
       y<2024 || y>2100 || m<1 || m>12 || d<1 || d>31 || h<0 || h>23 || min<0 || min>59) return 0;
    struct tm t{}; t.tm_year=y-1900; t.tm_mon=m-1; t.tm_mday=d; t.tm_hour=h; t.tm_min=min; t.tm_isdst=-1;
    time_t epoch=mktime(&t);
    if(t.tm_year!=y-1900 || t.tm_mon!=m-1 || t.tm_mday!=d || t.tm_hour!=h || t.tm_min!=min) return 0;
    return epoch>=ReminderClockMinimum?epoch:0;
}
struct ScheduledReminder { char id[17]{},text[33]{}; int64_t due=0,delivered=0; ReminderRepeat repeat=ReminderRepeat::Once; bool active=true,pending=false; int hour=-1,minute=-1; NoteReminder previous; };
struct ReminderNotice { bool active=false,work=false; char id[17]{},text[65]{}; int64_t due=0,snoozedUntil=0; };
struct ReminderRecord {
    uint32_t version=2;
    ScheduledReminder items[MaxNotes];
    ReminderNotice notice;
    int64_t lastWorkSlot=0; // Unused; kept so saved reminder state keeps its layout.
};
inline void advanceScheduledReminder(ScheduledReminder& item,int64_t now) {
    if(!item.pending){item.previous.due=item.due;item.previous.repeat=item.repeat;item.previous.active=item.active;item.previous.hour=item.hour;item.previous.minute=item.minute;}
    item.delivered=item.due;
    NoteReminder anchor;anchor.due=item.due;anchor.hour=item.hour;anchor.minute=item.minute;reminderTime(anchor);item.hour=anchor.hour;item.minute=anchor.minute;
    int64_t next=nextReminderDue(item.due,item.repeat,now,item.hour,item.minute);
    if(next)item.due=next;else item.active=false;
    item.pending=true;
}
class ReminderEngine {
public:
    ReminderRecord record;
    bool waitingForTime=true;
    bool visible(int64_t now,bool synchronized=true) const {
        return record.notice.active && (!record.notice.snoozedUntil || (synchronized && now>=ReminderClockMinimum && now>=record.notice.snoozedUntil));
    }
    bool sync(const NoteStorageState& state) {
        if(!state.ok || !state.listValid || state.busy || state.operation==NoteOperation::Read) return false;
        bool changed=false;
        for(auto& item:record.items) {
            bool present=false;
            for(unsigned i=0;i<state.count;++i) if(!strcmp(item.id,state.entries[i].id) && state.entries[i].reminderAt) present=true;
            if(item.id[0] && !present && !state.truncated) { item=ScheduledReminder{}; changed=true; }
        }
        for(unsigned i=0;i<state.count;++i) {
            const auto& entry=state.entries[i]; if(!entry.reminderAt) continue;
            ScheduledReminder* slot=nullptr;
            for(auto& item:record.items) if(!strcmp(item.id,entry.id)) { slot=&item; break; }
            if(!slot) for(auto& item:record.items) if(!item.id[0]) { slot=&item; break; }
            if(!slot) continue;
            NoteReminder incoming;incoming.due=entry.reminderAt;incoming.repeat=entry.repeat;incoming.active=entry.reminderActive;incoming.hour=entry.reminderHour;incoming.minute=entry.reminderMinute;
            NoteReminder desired;desired.due=slot->due;desired.repeat=slot->repeat;desired.active=slot->active;desired.hour=slot->hour;desired.minute=slot->minute;
            if(slot->pending && sameReminder(incoming,slot->previous)) {
                if(strcmp(slot->text,entry.title)){strlcpy(slot->text,entry.title,sizeof(slot->text));changed=true;}
                continue; // An older SD snapshot cannot undo a locally persisted delivery.
            }
            if(slot->pending){slot->pending=false;changed=true;}
            if(!sameReminder(desired,incoming) || strcmp(slot->text,entry.title) || strcmp(slot->id,entry.id)) {
                if(!sameReminder(desired,incoming))slot->delivered=0;
                strlcpy(slot->id,entry.id,sizeof(slot->id));strlcpy(slot->text,entry.title,sizeof(slot->text));
                slot->due=incoming.due;slot->repeat=incoming.repeat;slot->active=incoming.active;slot->hour=incoming.hour;slot->minute=incoming.minute;changed=true;
            }
        }
        if(record.notice.active && !record.notice.work) {
            if(state.operation==NoteOperation::Reminder && !strcmp(state.id,record.notice.id)){record.notice=ReminderNotice{};changed=true;}
            bool present=false; for(const auto& item:record.items) if(!strcmp(item.id,record.notice.id)) present=true;
            if(!present) { record.notice=ReminderNotice{}; changed=true; }
        }
        return changed;
    }
    bool tick(int64_t now,bool synchronized=true) {
        waitingForTime=!synchronized || now<ReminderClockMinimum; if(waitingForTime) return false;
        bool changed=false;
        if(!reminderWeekday(now))for(auto& item:record.items)if(item.id[0] && item.active && item.repeat==ReminderRepeat::Weekdays && item.due<=now){advanceScheduledReminder(item,now);changed=true;}
        if(!record.notice.active) {
            ScheduledReminder* next=nullptr;
            for(auto& item:record.items)
                if(item.id[0] && item.active && item.due<=now && item.delivered!=item.due && (!next || item.due<next->due)) next=&item;
            if(next) {
                record.notice=ReminderNotice{}; record.notice.active=true; record.notice.due=next->due;
                strlcpy(record.notice.id,next->id,sizeof(record.notice.id)); strlcpy(record.notice.text,next->text,sizeof(record.notice.text));
                advanceScheduledReminder(*next,now);changed=true;
            }
        }
        return changed;
    }
    void dismiss() { record.notice=ReminderNotice{}; }
    bool snooze(int64_t now,unsigned minutes) {
        if(!record.notice.active || waitingForTime || now<ReminderClockMinimum || (minutes!=10 && minutes!=30)) return false;
        record.notice.snoozedUntil=now+minutes*60; return true;
    }
};
}
