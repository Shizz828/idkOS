#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
namespace spark {
enum class ReminderRepeat : uint8_t { Once,Daily,Weekly,Weekdays };
inline const char* repeatName(ReminderRepeat repeat) {
    const char* names[]={"Once","Daily","Weekly","Weekdays"};
    return names[unsigned(repeat)<4?unsigned(repeat):0];
}
struct NoteReminder {
    int64_t due=0;ReminderRepeat repeat=ReminderRepeat::Once;bool active=true;int hour=-1,minute=-1;
};
inline void reminderTime(NoteReminder& value) {
    if(value.hour>=0 && value.minute>=0)return;
    time_t epoch=value.due;struct tm local{};localtime_r(&epoch,&local);value.hour=local.tm_hour;value.minute=local.tm_min;
}
inline bool sameReminder(NoteReminder a,NoteReminder b) {reminderTime(a);reminderTime(b);return a.due==b.due && a.repeat==b.repeat && a.active==b.active && a.hour==b.hour && a.minute==b.minute;}
inline bool validReminder(const NoteReminder& value) {return value.due>=1704067200 && value.due<=4133980799LL && unsigned(value.repeat)<4 && ((value.hour==-1 && value.minute==-1) || (value.hour>=0 && value.hour<24 && value.minute>=0 && value.minute<60));}
inline bool decodeNoteReminder(const char* data,NoteReminder& out) {
    long long due=0;unsigned repeat=0,active=1;int n=0,hour=-1,minute=-1;
    if(sscanf(data,"2 %lld %u %u %d %d%n",&due,&repeat,&active,&hour,&minute,&n)==5 && data[n]==0) {
        if(repeat>3 || active>1)return false;
    } else {char* end=nullptr;due=strtoll(data,&end,10);if(end==data || *end)return false;repeat=0;active=1;}
    NoteReminder next;next.due=due;next.repeat=ReminderRepeat(repeat);next.active=active;next.hour=hour;next.minute=minute;
    if(!validReminder(next))return false;
    out=next;return true;
}
inline void encodeNoteReminder(const NoteReminder& value,char* data,size_t size) {
    NoteReminder normalized=value;reminderTime(normalized);
    snprintf(data,size,"2 %lld %u %u %d %d",(long long)value.due,unsigned(value.repeat),value.active?1:0,normalized.hour,normalized.minute);
}
inline bool reminderWeekday(int64_t due) {time_t epoch=due;struct tm local{};localtime_r(&epoch,&local);return local.tm_wday!=0 && local.tm_wday!=6;}
inline void encodeReminderAdvance(const NoteReminder& old,const NoteReminder& next,char* data,size_t size) {
    snprintf(data,size,"%lld %u %u %d %d %lld %u %u %d %d",(long long)old.due,unsigned(old.repeat),old.active?1:0,old.hour,old.minute,(long long)next.due,unsigned(next.repeat),next.active?1:0,next.hour,next.minute);
}
inline bool decodeReminderAdvance(const char* data,NoteReminder& old,NoteReminder& next) {
    long long oldDue,newDue;unsigned oldRepeat,oldActive,repeat,active;int n=0;
    if(sscanf(data,"%lld %u %u %d %d %lld %u %u %d %d%n",&oldDue,&oldRepeat,&oldActive,&old.hour,&old.minute,&newDue,&repeat,&active,&next.hour,&next.minute,&n)!=10 || data[n] || oldRepeat>3 || repeat>3 || oldActive>1 || active>1)return false;
    old.due=oldDue;old.repeat=ReminderRepeat(oldRepeat);old.active=oldActive;
    next.due=newDue;next.repeat=ReminderRepeat(repeat);next.active=active;
    return validReminder(old) && validReminder(next);
}
// Advance by local calendar days, not 86400-second intervals. tm_isdst=-1
// keeps the selected wall-clock time through New York DST transitions.
inline int64_t nextReminderDue(int64_t due,ReminderRepeat repeat,int64_t now,int hour=-1,int minute=-1) {
    if(repeat==ReminderRepeat::Once || unsigned(repeat)>3 || now<1704067200)return 0;
    time_t original=due,current=now;struct tm wanted{},candidate{};
    localtime_r(&original,&wanted);localtime_r(&current,&candidate);
    if(hour>=0 && minute>=0){wanted.tm_hour=hour;wanted.tm_min=minute;}
    candidate.tm_hour=wanted.tm_hour;candidate.tm_min=wanted.tm_min;candidate.tm_sec=0;
    if(repeat==ReminderRepeat::Weekly)candidate.tm_mday+=(wanted.tm_wday-candidate.tm_wday+7)%7;
    // Fall-back repeats an hour, not a calendar day: never schedule the same
    // local date twice after delivering its occurrence.
    if(candidate.tm_year==wanted.tm_year && candidate.tm_yday==wanted.tm_yday && now>=due)candidate.tm_mday+=repeat==ReminderRepeat::Weekly?7:1;
    for(unsigned attempt=0;attempt<9;++attempt) {
        candidate.tm_isdst=-1;time_t next=mktime(&candidate);
        bool weekday=candidate.tm_wday!=0 && candidate.tm_wday!=6;
        if(next>now && (repeat!=ReminderRepeat::Weekdays || weekday))return next<=4133980799LL?next:0;
        // Reset the desired time after mktime normalizes a spring-forward gap.
        candidate.tm_mday+=repeat==ReminderRepeat::Weekly?7:1;
        candidate.tm_hour=wanted.tm_hour;candidate.tm_min=wanted.tm_min;candidate.tm_sec=0;
    }
    return 0;
}
}
