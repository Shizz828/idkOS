#pragma once
#include <stdint.h>
namespace spark {
class ReminderTone {
    bool playing=false;
    uint32_t started=0;
    uint8_t previousVolume=0;
public:
    bool active()const{return playing;}
    template<typename Speaker> void play(uint32_t now,Speaker& speaker) {
        if(playing || !speaker.begin())return;
        previousVolume=speaker.getVolume();speaker.setVolume(48);
        speaker.tone(880.0f,240,0,true);started=now;playing=true;
    }
    template<typename Speaker> void update(uint32_t now,Speaker& speaker) {
        if(playing && uint32_t(now-started)>=260){speaker.stop(0);speaker.setVolume(previousVolume);playing=false;}
    }
};
}
