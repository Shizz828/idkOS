#pragma once
#include <M5Cardputer.h>
#include "Navigation.h"
namespace spark {
struct InputEvent {
    bool up=false, down=false, left=false, right=false;
    bool enter=false, back=false, home=false, erase=false;
    bool shift=false, fn=false, changed=false;
    char shortcut=0,control=0;
    bool tab=false;
    bool save=false,enterDown=false,reminder=false,sleep=false,idle=false,keyPressed=false;
    char text[57]{};
};
class Input {
    bool previous[56]{};
    bool enterHeld=false;
    bool saveHeld=false,reminderHeld=false,sleepHeld=false,idleHeld=false;
    bool shortcutHeld[7]{};
    bool escPending=false,escUsed=false;
    uint32_t escStarted=0;
    RepeatKey arrows[4], erase;
public:
    // Menus and lists use the arrow keys without Fn; Fn+arrow then types the
    // printed character. Terminal, SSH and text entry keep the Fn-layer arrows.
    // textEntry (Terminal, SSH and any text field): Shift+Esc types '~'.
    // Everywhere Esc is Back and Fn+Esc is Sleep.
    InputEvent poll(uint32_t now,bool textEntry=false,bool menuMode=false) {
        InputEvent out;
        const auto& s = M5Cardputer.Keyboard.keysState();
        out.shift = s.shift;
        out.fn = s.fn;
        bool current[56]{};
        unsigned n = 0;
        for (const auto& key : M5Cardputer.Keyboard.keyList()) {
            if (key.x < 0 || key.x >= 14 || key.y < 0 || key.y >= 4) continue;
            unsigned id = key.y * 14 + key.x;
            current[id] = true;
            // Special keys use HID codes that overlap printable ASCII:
            // Backspace=42 ('*'), Tab=43 ('+'), Enter=40 ('(').
            // Filter their physical positions before converting to text.
            if ((key.y == 0 && (key.x == 13 || key.x == 0)) ||
                (key.y == 1 && key.x == 0) ||
                (key.y == 2 && key.x == 13)) continue;
            bool arrowKey=(key.y==2 && key.x==11) || (key.y==3 && key.x>=10 && key.x<=12);
            if(menuMode && arrowKey) {
                if(!previous[id] && s.fn && !s.alt && !s.ctrl && n<56)out.text[n++]=char(M5Cardputer.Keyboard.getKey(key));
                continue;
            }
            if (previous[id] || s.fn || s.alt) continue;
            unsigned char c = M5Cardputer.Keyboard.getKey(key);
            if(s.ctrl){if(c>='a'&&c<='z')c-=32;if(c>='@'&&c<='_')out.control=char(c&31);continue;}
            if (c >= 32 && c <= 126 && n < 56) out.text[n++] = char(c);
        }
        // Published M5Cardputer 1.1.1 has no Fn-layer arrow/Escape fields.
        // Decode the printed Fn controls from physical positions, independent
        // of Shift and the library's version-specific special-key names.
        bool arrowLayer=menuMode?!s.fn:s.fn;
        out.up = arrows[0].update(arrowLayer && current[2*14+11], now);
        out.down = arrows[1].update(arrowLayer && current[3*14+11], now);
        out.left = arrows[2].update(arrowLayer && current[3*14+10], now);
        out.right = arrows[3].update(arrowLayer && current[3*14+12], now);
        out.erase = erase.update(!s.fn && current[13], now);
        bool enterDown = !s.fn && current[2*14+13];
        bool sleepDown=s.fn && current[0];
        out.sleep=sleepDown && !sleepHeld;sleepHeld=sleepDown;
        bool idleDown=s.fn && !s.ctrl && !s.alt && current[2*14+10]; // Fn+L
        out.idle=idleDown && !idleHeld;idleHeld=idleDown;
        // Resolve modifiers before navigation. ADV queues individual physical
        // key events: allow a short chord window in text entry so Esc cannot
        // navigate before a queued Shift/Fn event reaches keysState().
        if(!textEntry)escPending=false;
        if(current[0] && !previous[0]) {
            escUsed=false;escStarted=now;
            escPending=textEntry && !s.fn && !s.shift;
        }
        if(current[0] && s.fn){escPending=false;escUsed=true;}
        else if(current[0] && textEntry && s.shift) {
            escPending=false;
            if(!escUsed && !s.ctrl && !s.alt && n<56)out.text[n++]='~';
            escUsed=true;
        }
        if(textEntry) {
            if(escPending && (!current[0] || uint32_t(now-escStarted)>=35)) {
                out.back=true;escPending=false;escUsed=true;
            }
        } else out.back=!s.fn && current[0] && !previous[0];
        out.tab=!s.fn && current[14] && !previous[14];
        out.enterDown=enterDown;
        out.enter = enterDown && !enterHeld;
        enterHeld = enterDown;
        out.home = M5Cardputer.BtnA.wasPressed();
        bool saveDown=s.ctrl && !s.fn && current[2*14+3];
        out.save=saveDown && !saveHeld; saveHeld=saveDown;
        bool reminderDown=s.ctrl && !s.fn && current[1*14+4];
        out.reminder=reminderDown && !reminderHeld; reminderHeld=reminderDown;
        // Chord edges support either key order and never auto-repeat launches.
        const unsigned positions[]={1,2,3,4,5,6,2*14+3};
        const char shortcuts[]={'1','2','3','4','5','6','s'};
        for(unsigned i=0;i<7;++i) {
            bool down=s.fn && !s.ctrl && !s.alt && current[positions[i]];
            if(down && !shortcutHeld[i] && !out.shortcut) out.shortcut=shortcuts[i];
            shortcutHeld[i]=down;
        }
        for (unsigned i=0; i<56; ++i) {
            if(current[i] && !previous[i])out.keyPressed=true;
            if (previous[i] != current[i]) out.changed = true;
            previous[i] = current[i];
        }
        return out;
    }
};
}
