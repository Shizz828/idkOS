#pragma once
#include <stdint.h>
namespace spark {
// M5GFX::begin() is a no-op once the board is detected. Reset the existing
// panel instead: this also reattaches its PWM backlight after a warm reset.
template<typename Display> bool recoverDisplay(Display& display,uint8_t brightness) {
    display.waitDisplay();
    const bool inverted=display.getInvert();
    const auto depth=display.getColorDepth();
    if(!display.panel()->init(true))return false;
    // Logical inversion is separate from the panel's physical inversion config.
    // Preserve the logical state; forcing true reverses black/white on ADV.
    display.invertDisplay(inverted);
    display.setColorDepth(depth);
    display.setRotation(1);
    display.setTextWrap(false);
    display.clearClipRect();
    display.powerSaveOff();
    display.wakeup();
    display.setBrightness(brightness);
    return true;
}
}
