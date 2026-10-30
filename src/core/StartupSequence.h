#pragma once
#include <stdint.h>
namespace spark {
constexpr bool StartupSoundEnabled=true;
constexpr uint32_t StartupDurationMs=900;
// 5% above the previous 48/255 target, rounded to the nearest API step.
constexpr uint8_t StartupChimeVolume=(48u*105u+50u)/100u;
constexpr uint32_t StartupAudioWarmupMs=300;
class StartupSequence {
    enum class Audio { Off, Warming, First, Second };
    uint32_t started=0,stageStarted=0;
    bool active=false;
    Audio audio=Audio::Off;
    uint8_t previousVolume=0;
    const char* result="disabled";
    template<typename Speaker> void finishSound(Speaker& speaker,const char* status) {
        speaker.stop(0);speaker.setVolume(previousVolume);audio=Audio::Off;result=status;
    }
public:
    bool showing() const {return active;}
    bool soundActive() const {return audio!=Audio::Off;}
    const char* audioStatus() const {return result;}
    template<typename Speaker> void begin(uint32_t now,Speaker& speaker,bool sound=StartupSoundEnabled) {
        started=stageStarted=now;active=true;audio=Audio::Off;result="disabled";
        if(!sound)return;
        if(!speaker.begin()){result="begin-failed";return;}
        previousVolume=speaker.getVolume();speaker.setVolume(0);
        // A delay alone does not guarantee I2S is transmitting. Feed signed
        // zero PCM for 256 ms so the ADV codec has real clocks while settling.
        // Static storage stays valid until the asynchronous playback consumes it.
        static const int8_t silence[128]={};
        audio=Audio::Warming;result="warming";
        if(!speaker.playRaw(silence,sizeof(silence),16000,false,32,0,true))
            finishSound(speaker,"prime-failed");
    }
    template<typename Speaker> bool update(uint32_t now,Speaker& speaker) {
        if(!active)return false;
        uint32_t age=now-stageStarted;
        if(audio!=Audio::Off) {
            bool playing=speaker.isPlaying(0)!=0;
            // An unavailable/stalled playback task must never hold the intro.
            if(playing && age>=1500)finishSound(speaker,"playback-timeout");
            else if(audio==Audio::Warming && age>=StartupAudioWarmupMs && !playing) {
                speaker.setVolume(StartupChimeVolume);
                if(speaker.tone(1046.5f,120,0,true)) {
                    audio=Audio::First;stageStarted=now;result="first-tone";
                } else finishSound(speaker,"tone-failed");
            } else if(audio==Audio::First && age>=200 && !playing) {
                if(speaker.tone(784.0f,140,0,true)) {
                    audio=Audio::Second;stageStarted=now;result="second-tone";
                } else finishSound(speaker,"tone-failed");
            } else if(audio==Audio::Second && age>=160 && !playing)
                finishSound(speaker,"complete");
        }
        if(uint32_t(now-started)<StartupDurationMs || soundActive())return false;
        active=false;return true;
    }
};
}
