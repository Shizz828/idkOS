#include <initializer_list>
#include <cassert>
#include <cstdlib>
#include "../src/core/ReminderEngine.h"
using namespace spark;
int main(){
 setenv("TZ","EST5EDT,M3.2.0/2,M11.1.0/2",1);tzset();
 auto date=[](const char* text){return parseReminderDate(text);};
 auto friday=date("2026-10-16 18:30");
 assert(nextReminderDue(friday,ReminderRepeat::Once,friday)==0);
 assert(nextReminderDue(friday,ReminderRepeat::Daily,friday)==date("2026-10-17 18:30"));
 assert(nextReminderDue(friday,ReminderRepeat::Weekly,friday)==date("2026-10-23 18:30"));
 assert(nextReminderDue(friday,ReminderRepeat::Weekdays,friday)==date("2026-10-19 18:30"));
 assert(nextReminderDue(friday,ReminderRepeat::Daily,date("2026-12-31 23:59"))==date("2027-01-01 18:30"));
 assert(nextReminderDue(date("2026-03-07 09:30"),ReminderRepeat::Daily,date("2026-03-07 09:30"))==date("2026-03-08 09:30"));
 assert(nextReminderDue(date("2026-10-31 09:30"),ReminderRepeat::Daily,date("2026-10-31 09:30"))==date("2026-11-01 09:30"));
 auto fold=date("2026-11-01 01:30");assert(nextReminderDue(fold,ReminderRepeat::Daily,fold,1,30)==date("2026-11-02 01:30"));
 // A nonexistent 02:30 shifts forward on spring day, then returns to 02:30.
 auto gap=nextReminderDue(date("2026-03-07 02:30"),ReminderRepeat::Daily,date("2026-03-07 02:30"),2,30);
 assert(gap==date("2026-03-08 03:30"));assert(nextReminderDue(gap,ReminderRepeat::Daily,gap,2,30)==date("2026-03-09 02:30"));
 NoteReminder old;assert(decodeNoteReminder("1792193400",old));assert(old.repeat==ReminderRepeat::Once && old.active);
 NoteReminder value;value.due=friday;value.repeat=ReminderRepeat::Weekdays;value.active=false;value.hour=18;value.minute=30;
 char encoded[64];encodeNoteReminder(value,encoded,sizeof(encoded));NoteReminder loaded;assert(decodeNoteReminder(encoded,loaded));assert(sameReminder(value,loaded));
 for(const char* invalid:{"junk","0","2 1792193400 4 1 18 30","2 1792193400 1 2 18 30","2 1792193400 1 1 25 30","1792193400 junk"})assert(!decodeNoteReminder(invalid,loaded));
 NoteStorageState state;state.ok=state.listValid=true;state.count=1;strcpy(state.entries[0].id,"0123456789abcdef");strcpy(state.entries[0].title,"Recurring Spark");state.entries[0].reminderAt=friday;state.entries[0].repeat=ReminderRepeat::Daily;
 ReminderEngine engine;assert(engine.sync(state));assert(!engine.tick(friday,false));assert(engine.tick(friday,true));assert(engine.record.notice.active && !engine.record.notice.work);auto& item=engine.record.items[0];assert(item.pending && item.due==date("2026-10-17 18:30"));
 engine.sync(state);assert(item.pending && item.due==date("2026-10-17 18:30")); // stale SD cannot roll back
 assert(engine.snooze(friday,30));assert(item.due==date("2026-10-17 18:30"));
 state.entries[0].reminderAt=item.due;state.entries[0].reminderHour=item.hour;state.entries[0].reminderMinute=item.minute;assert(engine.sync(state));assert(!item.pending);
 engine.dismiss();assert(!engine.tick(friday+1800));assert(engine.tick(item.due));assert(item.due==date("2026-10-18 18:30"));
 // User edits supersede queued automatic writeback.
 state.entries[0].reminderAt=date("2026-10-20 18:30");state.entries[0].reminderActive=false;engine.sync(state);assert(!item.pending && !item.active);engine.dismiss();assert(!engine.tick(state.entries[0].reminderAt));
 state.entries[0].repeat=ReminderRepeat::Once;state.entries[0].reminderActive=true;engine.sync(state);assert(engine.tick(state.entries[0].reminderAt));assert(!item.active && item.pending);engine.dismiss();assert(!engine.tick(state.entries[0].reminderAt+60));
 // A partial listing is not evidence that an existing reminder was deleted.
 state.count=0;state.truncated=true;engine.sync(state);assert(item.id[0]);state.truncated=false;engine.sync(state);assert(!item.id[0]);
 state.count=1;state.entries[0].reminderAt=friday;state.entries[0].repeat=ReminderRepeat::Weekdays;state.entries[0].reminderActive=true;
 ReminderEngine weekend;weekend.sync(state);assert(weekend.tick(date("2026-10-17 18:30")));assert(!weekend.record.notice.active);assert(weekend.record.items[0].due==date("2026-10-19 18:30"));

}
