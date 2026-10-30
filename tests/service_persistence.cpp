#include <cassert>
#include <cstdlib>
#include "../src/services/ReminderService.h"
#include "../src/services/KnownNetworks.h"
int main() {
    setenv("TZ","EST5EDT,M3.2.0,M11.1.0",1); tzset();
    int64_t now=spark::parseReminderDate("2026-10-13 09:00");
    spark::NoteStorageState once; once.ok=once.listValid=true; once.count=1; strcpy(once.entries[0].id,"fedcba9876543210"); strcpy(once.entries[0].title,"Water the plants"); once.entries[0].reminderAt=now;
    spark::ReminderService service; service.begin(); service.sync(once); assert(service.poll(now));
    service.engine.snooze(now,10); service.save(); assert(!service.saveFailed);
    spark::ReminderService restored; restored.begin();
    assert(restored.engine.record.notice.active && !restored.engine.visible(now+599));
    assert(restored.engine.visible(now+600)); restored.engine.dismiss(); restored.save();
    spark::ReminderService dismissed; dismissed.begin(); assert(!dismissed.engine.record.notice.active);
    auto& bytes=Preferences::data()["spark-reminders/state"]; bytes[0]^=1;
    spark::ReminderService corrupt; corrupt.begin(); assert(!corrupt.engine.record.notice.active);
    spark::KnownNetworks networks; networks.begin(); assert(networks.count()==0);
    networks.remember("home","test-home"); networks.remember("hotspot","test-hotspot");
    networks.remember("office","test-office"); networks.remember("fourth","test-fourth");
    assert(networks.count()==4); spark::KnownNetworks saved; saved.begin();
    char ssid[33]{},password[65]{};
    assert(saved.candidate(ssid,password) && !strcmp(ssid,"fourth") && !strcmp(password,"test-fourth"));
    assert(saved.candidate(ssid,password) && !strcmp(ssid,"office"));
    assert(saved.candidate(ssid,password) && !strcmp(ssid,"hotspot"));
    assert(saved.matching("office",ssid,password)&&!strcmp(password,"test-office"));
    assert(!saved.matching("missing",ssid,password));
    char names[5][33]{};saved.names(names);assert(!strcmp(names[1],"office"));
    assert(saved.forget(1));spark::KnownNetworks forgotten;forgotten.begin();assert(forgotten.count()==3);
    assert(forgotten.selected(1,ssid,password)&&!strcmp(ssid,"hotspot"));assert(!forgotten.forget(3));
    Preferences::writesFail()=true;
    assert(!forgotten.remember("unsaved","test-password") && forgotten.count()==3);
    assert(!forgotten.forget(0) && forgotten.count()==3);
    Preferences::writesFail()=false;
    spark::KnownNetworks afterFailure;afterFailure.begin();assert(afterFailure.count()==3);
    // Fill all five slots, then verify oldest eviction and access to slot 5.
    assert(afterFailure.remember("fifth","five"));assert(afterFailure.remember("sixth","six"));
    assert(afterFailure.count()==5);assert(afterFailure.selected(4,ssid,password));
    assert(afterFailure.remember("seventh","seven"));assert(!afterFailure.matching("home",ssid,password));
    spark::KnownNetworks five;five.begin();assert(five.count()==5);assert(five.forget(4));
    auto& profiles=Preferences::data()["spark-networks/profiles"]; profiles.back()^=1;
    spark::KnownNetworks damaged; damaged.begin(); assert(damaged.count()==0);
    // Preserve version-1 credentials when upgrading from three slots.
    struct Entry {char ssid[33]{},password[65]{};};
    struct Legacy {uint32_t version=1;Entry entries[3];uint32_t checksum=0;} legacy;
    strcpy(legacy.entries[0].ssid,"legacy");strcpy(legacy.entries[0].password,"secret");
    auto raw=reinterpret_cast<const uint8_t*>(&legacy);uint32_t hash=2166136261u;
    for(size_t i=0;i<offsetof(Legacy,checksum);++i)hash=(hash^raw[i])*16777619u;
    legacy.checksum=hash;Preferences prefs;prefs.begin("spark-networks",false);prefs.putBytes("profiles",&legacy,sizeof(legacy));
    spark::KnownNetworks migrated;migrated.begin();assert(migrated.count()==1);
    assert(migrated.selected(0,ssid,password) && !strcmp(ssid,"legacy") && !strcmp(password,"secret"));
    assert(migrated.remember("new","new-secret"));spark::KnownNetworks upgraded;upgraded.begin();assert(upgraded.count()==2);
}
