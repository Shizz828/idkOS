#pragma once
#include <M5Cardputer.h>
#include <memory>
#include "utility/Adafruit_TCA8418/Adafruit_TCA8418.h"
#include "KeyEvent.h"
#include "LightSleep.h"
namespace spark {
// The ADV keyboard, read from the level of its IRQ pin (see KeyEvent.h).
// M5Cardputer 1.1.1's reader keeps the edge in a flag, which is what lost it.
class LevelKeyboardReader : public KeyboardReader {
    Adafruit_TCA8418 chip;
public:
    void begin() override {
        chip.begin();chip.matrix(7,8);chip.flush();
        pinMode(StandbyKeyboardPin,INPUT);
        chip.enableInterrupts();
    }
    void update() override {readKeyEvent(chip,!gpio_get_level(StandbyKeyboardPin),_key_list);}
};
// The library's reader must lose its interrupt first: the handler points at it.
inline void useLevelKeyboard() {
    detachInterrupt(digitalPinToInterrupt(StandbyKeyboardPin));
    M5Cardputer.Keyboard.begin(std::unique_ptr<KeyboardReader>(new LevelKeyboardReader));
}
}
