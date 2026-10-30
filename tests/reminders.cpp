#include <cassert>
#include <cstdlib>
#include "../src/core/ReminderEngine.h"
using namespace spark;
int main() {
 setenv("TZ","EST5EDT,M3.2.0/2,M11.1.0/2",1); tzset();
 auto monday=parseReminderDate("2026-10-12 07:00"); assert(monday);
 assert(!parseReminderDate("2026-02-30 07:00")); assert(!parseReminderDate("2026-03-08 02:30"));
 ReminderEngine engine; assert(!engine.tick(0) && engine.waitingForTime);
 // No built-in alerts: without reminders the engine stays quiet all week, at any hour.
 for(int64_t t=monday-7*3600;t<monday+7*86400;t+=1800){assert(!engine.tick(t));assert(!engine.record.notice.active);}
 ReminderEngine restored;
 NoteStorageState state; state.ok=state.listValid=true; state.count=1;
 strcpy(state.entries[0].id,"0123456789abcdef"); strcpy(state.entries[0].title,"Take notes");
 state.entries[0].reminderAt=monday+14460;
 assert(restored.sync(state)); assert(!restored.tick(monday+14459)); assert(restored.tick(monday+14460));
 assert(!restored.record.notice.work && !strcmp(restored.record.notice.text,"Take notes"));
 assert(restored.snooze(monday+14460,10)); assert(!restored.visible(monday+15059)); assert(restored.visible(monday+15060));
 auto reboot=restored.record; ReminderEngine copy; copy.record=reboot; assert(copy.visible(monday+15060));
 restored.dismiss(); assert(!restored.tick(monday+14461));
 auto record=restored.record; ReminderEngine after; after.record=record; assert(!after.tick(monday+14462));
 state.listValid=false; state.count=0; assert(!after.sync(state)); assert(after.record.items[0].id[0]);
 state.listValid=true; assert(after.sync(state)); assert(!after.record.items[0].id[0]);
 // A snoozed reminder restored after reboot waits for time sync, then shows.
 ReminderEngine boot;boot.record=reboot;assert(!boot.visible(monday+15100,false));assert(!boot.tick(monday+15100,false));assert(!boot.snooze(monday+15100,10));
 assert(!boot.tick(monday+15100,true));assert(boot.visible(monday+15100,true));boot.dismiss();assert(!boot.tick(monday+15101,true));
 // A saved notice stays as it is across ticks.
 ReminderEngine note;note.record.notice.active=true;strcpy(note.record.notice.text,"Saved note");assert(!note.tick(monday));assert(note.record.notice.active && !note.record.notice.work);assert(!strcmp(note.record.notice.text,"Saved note"));

}
