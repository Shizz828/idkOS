#include <cassert>
#include <cstdlib>
#include "../src/core/StandbyPolicy.h"
#include "../src/core/NetworkStandby.h"
#include "../src/core/ScreenSleep.h"
#include "../src/core/ReminderEngine.h"
#include "../src/core/ReminderTone.h"
struct Speaker {
    int tones=0,volume=100;
    bool begin(){return true;}int getVolume(){return volume;}
    void setVolume(int v){volume=v;}void stop(int){}
    void tone(float,int,int,bool){++tones;}
};
int main(){
    using namespace spark;
    assert(StandbyIntervalUs==1000000);
    // Each prerequisite independently prevents sleeping over active work/input.
    for(unsigned mask=0;mask<64;++mask)
        assert(canEnterStandby(mask&1,!(mask&2),mask&4,mask&8,!(mask&16),!(mask&32))==(mask==63));
    NetworkStandby network;
    assert(!network.ready());network.request(true);auto first=network.ticket();
    assert(!network.ready());network.acknowledge(first);assert(network.ready());
    network.request(false);assert(!network.ready());network.request(true);
    assert(!network.ready());network.acknowledge(first);assert(!network.ready());
    network.acknowledge(network.ticket());assert(network.ready());
    network.request(true);assert(network.ready()); // Idempotent requests.
    ScreenSleep screen;bool wake=false;
    screen.filter(false,false,false,false,wake);
    screen.filter(true,true,false,true,wake);assert(!screen.readyForStandby());
    screen.filter(false,false,false,false,wake);assert(screen.readyForStandby());
    setenv("TZ","UTC0",1);tzset();
    auto monday=parseReminderDate("2026-10-12 07:00");
    ReminderEngine reminders;ReminderTone tone;Speaker speaker;
    NoteStorageState once;once.ok=once.listValid=true;once.count=1;strcpy(once.entries[0].id,"fedcba9876543210");strcpy(once.entries[0].title,"Water the plants");once.entries[0].reminderAt=monday;
    assert(reminders.sync(once));assert(reminders.tick(monday));tone.play(100,speaker);
    assert(tone.active() && speaker.tones==1);
    assert(!canEnterStandby(true,false,true,true,false,tone.active()));
    tone.update(360,speaker);assert(!tone.active());
    assert(reminders.snooze(monday,10));
    for(int seconds=1;seconds<600;++seconds){
        reminders.tick(monday+seconds);
        assert(!reminders.visible(monday+seconds));
        assert(screen.sleeping() && screen.readyForStandby());
    }
    reminders.tick(monday+600);assert(reminders.visible(monday+600));
    tone.play(600100,speaker);assert(speaker.tones==2 && tone.active());
    // Keyboard presses stay asleep; G0 wakes through the same filter and is consumed.
    assert(screen.filter(false,true,false,true,wake));assert(!wake && screen.sleeping());
    assert(screen.filter(false,false,true,true,wake,true));assert(wake && !screen.sleeping());
    assert(screen.filter(false,true,false,true,wake));assert(!wake);
}
