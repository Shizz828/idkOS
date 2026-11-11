#pragma once
#include <M5Cardputer.h>
#include <string.h>
#include "Theme.h"
namespace spark {
constexpr int ScreenWidth=240,ScreenHeight=135,ScreenStripRows=8;
inline lgfx::LGFXBase*& screenTarget() {
    static lgfx::LGFXBase* target=&M5Cardputer.Display; return target;
}
// Apps retain full-screen coordinates; only the rendering target is translated.
// This keeps every existing view render-only and preserves its colors/layout.
class ScreenSurface {
    int top=0;
    lgfx::LGFXBase& target(){return *screenTarget();}
public:
    void offset(int y){top=y;}
    void fillScreen(uint16_t c){target().fillScreen(c);}
    void setTextSize(int size){target().setTextSize(size);}
    void setTextColor(uint16_t fg,uint16_t bg){target().setTextColor(fg,bg);}
    void setCursor(int x,int y){target().setCursor(x,y-top);}
    void print(const char* value){target().print(value);}
    void drawFastHLine(int x,int y,int w,uint16_t c){target().drawFastHLine(x,y-top,w,c);}
    void drawFastVLine(int x,int y,int h,uint16_t c){target().drawFastVLine(x,y-top,h,c);}
    void fillRect(int x,int y,int w,int h,uint16_t c){target().fillRect(x,y-top,w,h,c);}
    void drawLine(int x,int y,int x2,int y2,uint16_t c){target().drawLine(x,y-top,x2,y2-top,c);}
    void drawCircle(int x,int y,int r,uint16_t c){target().drawCircle(x,y-top,r,c);}
    void fillCircle(int x,int y,int r,uint16_t c){target().fillCircle(x,y-top,r,c);}
    void fillTriangle(int x,int y,int x2,int y2,int x3,int y3,uint16_t c){target().fillTriangle(x,y-top,x2,y2-top,x3,y3-top,c);}
    void drawRect(int x,int y,int w,int h,uint16_t c){target().drawRect(x,y-top,w,h,c);}
    void fillRoundRect(int x,int y,int w,int h,int r,uint16_t c){target().fillRoundRect(x,y-top,w,h,r,c);}
    void drawRoundRect(int x,int y,int w,int h,int r,uint16_t c){target().drawRoundRect(x,y-top,w,h,r,c);}
    // Real font metrics for centring (the built-in font is 6x8 per size step).
    int textWidth(const char* value){return int(target().textWidth(value));}
    int fontHeight(){return int(target().fontHeight());}
    // Tiny 4-px font for labels that must fit under 38-px Home stations. Callers
    // always switch back (smallText does), so no other screen ever sees it.
    void smallFont(bool on){if(on)target().setFont(&fonts::TomThumb);else target().setFont(&fonts::Font0);}
};
inline ScreenSurface& screen(){static ScreenSurface surface;return surface;}
bool initScreenBuffer();
void prepareScreenStrip(int top);
void presentScreenStrip(int x,int top,int width,int rows);
// Callbacks draw cached UI state only: no input, networking or persistence.
// Each complete strip is assembled off-screen, then sent to the panel once.
template<typename Draw> void renderScreenRegion(int x,int y,int width,int height,const Draw& draw) {
    if(x<0){width+=x;x=0;}if(y<0){height+=y;y=0;}
    if(x>=ScreenWidth || y>=ScreenHeight || width<=0 || height<=0)return;
    if(width>ScreenWidth-x)width=ScreenWidth-x;
    if(height>ScreenHeight-y)height=ScreenHeight-y;
    for(int top=y;top<y+height;top+=ScreenStripRows) {
        int rows=y+height-top;if(rows>ScreenStripRows)rows=ScreenStripRows;
        prepareScreenStrip(top);draw();presentScreenStrip(x,top,width,rows);
    }
    screen().offset(0);
}
template<typename Draw> void renderScreen(const Draw& draw){renderScreenRegion(0,0,ScreenWidth,ScreenHeight,draw);}
inline void text(int x,int y,const char* value,int size=1,uint16_t color=theme::Content) {
    auto& d=screen();d.setTextSize(size);d.setTextColor(color,TFT_BLACK);d.setCursor(x,y);d.print(value);
}
// Text centred on cx using the font's measured width.
inline void centerText(int cx,int y,const char* value,int size=1,uint16_t color=theme::Content,uint16_t background=TFT_BLACK) {
    auto& d=screen();d.setTextSize(size);d.setTextColor(color,background);
    d.setCursor(cx-(d.textWidth(value)-size)/2,y);d.print(value);
}
// One character centred on (cx,cy) both ways. The cell's last column and row are
// spacing, so the visible glyph is (width-size) x (height-size).
inline void centerGlyph(int cx,int cy,char glyph,int size,uint16_t color,uint16_t background) {
    auto& d=screen();char value[2]={glyph,0};d.setTextSize(size);d.setTextColor(color,background);
    d.setCursor(cx-(d.textWidth(value)-size)/2,cy-(d.fontHeight()-size)/2);d.print(value);
}
// Small-font text centred on cx; restores the normal font.
inline void smallText(int cx,int y,const char* value,uint16_t color,uint16_t background=TFT_BLACK) {
    auto& d=screen();d.smallFont(true);d.setTextSize(1);d.setTextColor(color,background);
    d.setCursor(cx-(d.textWidth(value)-1)/2,y);d.print(value);d.smallFont(false);
}
inline void frame(const char* title) {
    screen().fillScreen(TFT_BLACK);text(6,5,title,strlen(title)>11?1:2,theme::Primary);
    screen().drawFastHLine(6,25,228,theme::Primary);
}
}
