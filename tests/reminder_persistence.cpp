#include <cassert>
#include <cstdlib>
#include "../src/services/ReminderService.h"
#include "../src/core/SystemTime.h"
using namespace spark;
struct PendingStorage {
 bool accept=false;unsigned calls=0;char payload[96]{};
 bool submit(NoteOperation op,const char*,const char* data){assert(op==NoteOperation::ReminderAdvance);++calls;if(!accept)return false;strcpy(payload,data);return true;}
};
struct OldItem {char id[17]{},text[33]{};int64_t due=0,delivered=0;};
struct OldRecord {uint32_t version=1;OldItem items[MaxNotes];ReminderNotice notice;int64_t lastWorkSlot=0;};
struct OldBlob {OldRecord record;uint32_t checksum=0;};
int main(){
 setenv("TZ","EST5EDT,M3.2.0/2,M11.1.0/2",1);tzset();
 auto due=parseReminderDate("2026-10-12 07:00");
 assert(!systemTimeReady(due));systemTimeSynchronized().store(true);assert(systemTimeReady(due));assert(!systemTimeReady(0));
 NoteStorageState once;once.ok=once.listValid=true;once.count=1;strcpy(once.entries[0].id,"fedcba9876543210");strcpy(once.entries[0].title,"Water the plants");once.entries[0].reminderAt=due;
 ReminderService service;service.begin();service.sync(once);assert(!service.poll(due,false));assert(service.poll(due,true));
 ReminderService reboot;reboot.begin();assert(reboot.engine.record.notice.active);assert(reboot.engine.record.notice.due==due);
 reboot.poll(due,true);assert(reboot.engine.snooze(due,10));reboot.save();
 ReminderService snoozed;snoozed.begin();assert(!snoozed.engine.visible(due+599));assert(snoozed.engine.visible(due+600));
 snoozed.engine.dismiss();snoozed.save();ReminderService dismissed;dismissed.begin();dismissed.poll(due+601,true);assert(!dismissed.engine.record.notice.active);
 once.count=2;strcpy(once.entries[1].id,"00112233445566aa");strcpy(once.entries[1].title,"Second");once.entries[1].reminderAt=due+7200;assert(dismissed.engine.sync(once));
 Preferences::writesFail()=true;assert(dismissed.poll(due+7200,true));assert(dismissed.saveFailed);Preferences::writesFail()=false;dismissed.save();assert(!dismissed.saveFailed);
 // Corrupt persistence must not restore arbitrary reminder state.
 Preferences::data()["spark-reminders/state"][0]^=1;ReminderService corrupt;corrupt.begin();assert(!corrupt.engine.record.notice.active);
 Preferences::data().clear();NoteStorageState state;state.ok=state.listValid=true;state.count=1;
 auto noteDue=parseReminderDate("2026-10-16 18:30");strcpy(state.entries[0].id,"0123456789abcdef");strcpy(state.entries[0].title,"Daily saved Spark");state.entries[0].reminderAt=noteDue;state.entries[0].repeat=ReminderRepeat::Daily;
 ReminderService notes;notes.sync(state);assert(notes.poll(noteDue,true));assert(notes.engine.record.items[0].pending);
 assert(Preferences::data()["spark-reminders/state"].size()<512); // compact, populated schedules only
 assert(notes.engine.snooze(noteDue,30));notes.save();ReminderService noteBoot;noteBoot.begin();assert(noteBoot.engine.record.notice.active);assert(!noteBoot.engine.visible(noteDue+1799));assert(noteBoot.engine.record.items[0].pending);
 noteBoot.sync(state);assert(noteBoot.engine.record.items[0].due==parseReminderDate("2026-10-17 18:30"));
 PendingStorage storage;noteBoot.flush(storage,100);assert(storage.calls==1 && !storage.payload[0]);storage.accept=true;noteBoot.flush(storage,200);assert(storage.calls==2);
 NoteReminder expected,next;assert(decodeReminderAdvance(storage.payload,expected,next));assert(expected.due==noteDue && next.due==parseReminderDate("2026-10-17 18:30") && next.repeat==ReminderRepeat::Daily && next.active);
 state.entries[0].reminderAt=next.due;state.entries[0].reminderHour=next.hour;state.entries[0].reminderMinute=next.minute;noteBoot.sync(state);assert(!noteBoot.engine.record.items[0].pending);
 noteBoot.engine.dismiss();noteBoot.save();ReminderService afterAck;afterAck.begin();assert(!afterAck.engine.record.notice.active);assert(afterAck.poll(next.due,true));
 // Upgrade preserves a notice saved by an older build (including its work flag), snooze and consumed once reminder.
 Preferences::data().clear();OldBlob legacy;legacy.record.notice.active=legacy.record.notice.work=true;legacy.record.notice.due=due;legacy.record.notice.snoozedUntil=due+600;legacy.record.lastWorkSlot=due;
 strcpy(legacy.record.items[0].id,"0123456789abcdef");legacy.record.items[0].due=legacy.record.items[0].delivered=noteDue;
 uint32_t hash=2166136261u;const auto* bytes=reinterpret_cast<const uint8_t*>(&legacy);for(size_t i=0;i<offsetof(OldBlob,checksum);++i)hash=(hash^bytes[i])*16777619u;legacy.checksum=hash;
 Preferences prefs;prefs.begin("spark-reminders",false);prefs.putBytes("state",&legacy,sizeof(legacy));prefs.end();
 ReminderService upgraded;upgraded.begin();assert(upgraded.engine.record.version==2);assert(upgraded.engine.record.notice.work && upgraded.engine.record.notice.snoozedUntil==due+600);assert(upgraded.engine.record.lastWorkSlot==due);assert(!upgraded.engine.record.items[0].active && upgraded.engine.record.items[0].pending);
 upgraded.save();ReminderService upgradedBoot;upgradedBoot.begin();assert(upgradedBoot.engine.record.notice.snoozedUntil==due+600);

}
