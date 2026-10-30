#pragma once
#include <vector>
#include <string>
struct Point2D_t {int x,y;};
struct KeyboardMock {
 struct State {bool fn=false,ctrl=false,alt=false,shift=false;} s;
 std::vector<Point2D_t> keys;
 const State& keysState(){return s;}
 const std::vector<Point2D_t>& keyList(){return keys;}
 unsigned char getKey(Point2D_t k){
  if(k.y==0 && k.x==13) return 42;
  if(k.y==1 && k.x==0) return 43;
  if(k.y==2 && k.x==13) return 40;
  if(k.y==2 && k.x==1) return 129;
  return k.x==2 ? (s.shift?'A':'a') : (s.shift?'B':'b');
 }
};
struct ButtonMock {bool wasPressed(){return false;}};

#pragma once
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdint>
constexpr uint16_t TFT_BLACK=0,TFT_WHITE=65535,TFT_CYAN=2047,TFT_GREEN=2016,TFT_RED=63488;
struct FakeDisplay {
 struct Printed {int x,y;std::string text;};
 std::vector<Printed> printed;
 int x=0,y=0,size=1;
 void point(int a,int b) { if(a<0 || a>=240 || b<0 || b>=135) fprintf(stderr,"Out of bounds: %d,%d cursor:%d,%d size:%d\n",a,b,x,y,size); assert(a>=0 && a<240 && b>=0 && b<135); }
 void fillScreen(uint16_t) {printed.clear();}
 void setTextSize(int n) { size=n; }
 void setTextColor(uint16_t,uint16_t) {}
 void setCursor(int a,int b) { x=a; y=b; point(a,b); }
 void print(const char* s) { printed.push_back({x,y,s});if(*s) { point(x+int(strlen(s))*6*size-1,y+8*size-1); } }
 void fillRect(int a,int b,int w,int h,uint16_t) { point(a,b); point(a+w-1,b+h-1); }
 void fillCircle(int a,int b,int r,uint16_t) { point(a-r,b-r); point(a+r,b+r); }
 void drawLine(int a,int b,int c,int d,uint16_t) { point(a,b); point(c,d); }
 void drawCircle(int a,int b,int r,uint16_t) { point(a-r,b-r); point(a+r,b+r); }
 void drawFastHLine(int a,int b,int length,uint16_t) { point(a,b); point(a+length-1,b); }
 void drawFastVLine(int a,int b,int length,uint16_t) { point(a,b); point(a,b+length-1); }
 void fillTriangle(int a,int b,int c,int d,int e,int f,uint16_t) { point(a,b); point(c,d); point(e,f); }
};


namespace lgfx { using LGFXBase=FakeDisplay; }

struct WifiTestDevice { KeyboardMock Keyboard; ButtonMock BtnA; FakeDisplay Display; } M5Cardputer;
