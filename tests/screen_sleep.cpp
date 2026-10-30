#include <cassert>
#include "../src/core/ScreenSleep.h"
#include "../src/ui/DisplayRecovery.h"
struct Display {
    int resets=0,brightness=0,rotation=0;bool asleep=true,powerSave=true,clipped=true,wrap=true;
    bool inverted=false;int depth=16,hardwareDepth=0;bool hardwareInverted=false;
    Display* panel(){return this;}
    void waitDisplay(){}
    bool init(bool reset){assert(reset);++resets;brightness=0;hardwareInverted=false;hardwareDepth=0;return true;}
    void setRotation(int r){rotation=r;}
    void setTextWrap(bool w){wrap=w;}
    void clearClipRect(){clipped=false;}
    bool getInvert(){return inverted;}
    int getColorDepth(){return depth;}
    void invertDisplay(bool invert){inverted=hardwareInverted=invert;}
    void setColorDepth(int value){depth=hardwareDepth=value;}
    void powerSaveOff(){powerSave=false;}
    void wakeup(){asleep=false;}
    void setBrightness(int b){brightness=b;}
};
int main(){
    spark::ScreenSleep s;bool wake=false;
    // A held boot key cannot reach a restart confirmation or an app.
    assert(s.filter(false,true,false,true,wake));assert(!wake);
    assert(s.filter(false,false,false,false,wake));
    assert(!s.filter(false,true,false,true,wake));
    Display d;assert(spark::recoverDisplay(d,80));assert(d.resets==1);
    for(int cycle=0;cycle<100;++cycle){
        assert(s.filter(true,true,false,true,wake));assert(s.sleeping());
        assert(s.filter(false,true,false,true,wake));assert(!wake); // pocket chord still held
        assert(s.filter(false,false,false,false,wake));assert(s.sleeping());
        // Keyboard presses never wake; only G0 does. Wake cannot dispatch app actions.
        for(int n=0;n<5;++n){assert(s.filter(false,true,false,n%2,wake));assert(!wake && s.sleeping());}
        assert(s.filter(false,cycle%2==0,true,true,wake));
        assert(wake && !s.sleeping());
        assert(spark::recoverDisplay(d,80));
        assert(!d.hardwareInverted && d.hardwareDepth==16); // Black remains black.
        assert(d.brightness==80 && d.rotation==1 && !d.asleep && !d.powerSave && !d.clipped && !d.wrap);
        for(int n=0;n<20;++n)assert(s.filter(n%2,n%2,false,true,wake) && !wake);
        assert(s.filter(false,false,false,false,wake));
        assert(!s.filter(false,true,false,true,wake));
    }
    assert(d.resets==101);
    assert(s.filter(true,true,false,true,wake));
    assert(s.filter(false,false,true,true,wake)); // G0 recovers even with stuck keys.
    assert(wake && !s.sleeping());
    assert(s.quarantined());
    assert(s.filter(false,true,false,true,wake)); // A stuck key keeps normal input blocked.
    assert(!s.filter(false,false,true,true,wake)); // A fresh G0 press still reaches Home.
    assert(!s.quarantined() && !wake);
    s.filter(true,true,false,true,wake);
    assert(s.filter(false,false,true,true,wake,true) && wake); // Physical G0 wakes.
    assert(s.filter(false,false,true,true,wake,true)); // Its delayed debounced edge is also consumed.
    assert(s.quarantined());
    assert(s.filter(false,false,false,true,wake,true)); // Still physically held.
    assert(s.filter(false,false,false,true,wake,false)); // Release G0; keyboard/IRQ remains stuck.
    assert(!s.filter(false,false,true,true,wake,true)); // Only a new press escapes.
    d.inverted=true;d.depth=24;
    assert(spark::recoverDisplay(d,80));
    assert(d.hardwareInverted && d.hardwareDepth==24); // Preserve either logical mode.
}
