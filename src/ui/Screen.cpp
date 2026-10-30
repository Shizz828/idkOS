#include "Screen.h"
namespace spark {
static M5Canvas buffer(&M5Cardputer.Display);
// Fixed storage: no large contiguous canvas allocation or restore retries.
// Nearly all the RAM recovered for Weather TLS remains available continuously.
alignas(4) static uint8_t stripPixels[ScreenWidth*ScreenStripRows];
bool initScreenBuffer() {
    buffer.setPsram(false);buffer.setColorDepth(8);
    buffer.setBuffer(stripPixels,ScreenWidth,ScreenStripRows);
    buffer.setTextWrap(false);screenTarget()=&buffer;return true;
}
void prepareScreenStrip(int top) {
    screen().offset(top);buffer.fillScreen(TFT_BLACK);
}
void presentScreenStrip(int x,int top,int width,int rows) {
    M5Cardputer.Display.setClipRect(x,top,width,rows);
    buffer.pushSprite(0,top);
    // Do not overwrite the strip while a DMA transfer might still read it.
    M5Cardputer.Display.waitDMA();M5Cardputer.Display.clearClipRect();
}
}
