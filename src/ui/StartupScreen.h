#pragma once
#include "Screen.h"
#include "BootLogo.h"
namespace spark {
// Standard boot screen: the idkOS thought-cloud logo centered on black. The strip
// renderer clears the screen; the 16-bit logo is then pushed straight to the panel
// once, because the 8-bit strip buffer would shift the burnt-orange OS.
inline void drawStartupScreen() {
    screen().fillScreen(theme::Background);
}
inline void presentStartupLogo() {
    auto& d=M5Cardputer.Display;const bool swapped=d.getSwapBytes();
    d.setSwapBytes(true);d.pushImage(BootLogoX,BootLogoY,BootLogoW,BootLogoH,BootLogoPixels);d.setSwapBytes(swapped);
}
}
