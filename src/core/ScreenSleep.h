#pragma once
namespace spark {
// Logical screen sleep; timed CPU sleeps are coordinated separately.
// Quarantine both the sleep chord and the entire wake gesture, including repeats.
class ScreenSleep {
    bool off=false,armed=false,blocked=true,homeReady=false;
public:
    // A due reminder lights the screen without a wake gesture to quarantine.
    bool reminderWake(){if(!off)return false;off=false;armed=false;blocked=false;homeReady=false;return true;}
    // An unattended reminder wake returns to the same screen sleep.
    bool reminderSleep(){if(off)return false;off=true;armed=false;return true;}
    // A dark idle board or automatic screen-off lit by input or motion consumes the whole wake gesture.
    void quarantineWake(){blocked=true;homeReady=false;}
    bool sleeping() const { return off; }
    bool quarantined() const { return blocked; }
    bool readyForStandby() const { return off && armed; }
    bool filter(bool sleep,bool /*pressed: ignored, only G0 wakes*/,bool home,bool anyHeld,bool& wake,bool homeHeld=false) {
        wake=false;
        if(off) {
            if(!anyHeld)armed=true;
            // Pocket-safe: keyboard presses are ignored; only G0 wakes, even with a stuck key.
            if(home) {
                off=false;blocked=true;wake=true;homeReady=!home && !homeHeld;
            }
            return true;
        }
        if(blocked) {
            // A fresh G0 press is an independent escape if a keyboard release
            // or IRQ is stuck after wake. The caller dispatches Home only.
            if(!homeHeld && !home)homeReady=true;
            if(home && homeReady){blocked=false;return false;}
            if(!anyHeld)blocked=false;
            return true;
        }
        if(sleep) { off=true;armed=false;return true; }
        return false;
    }
};
}
