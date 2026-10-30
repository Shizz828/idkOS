#pragma once
#include <vector>
struct Point2D_t {int x,y;};
struct KeyboardMock {
 struct State {bool fn=false,ctrl=false,alt=false,shift=false;} s;
 std::vector<Point2D_t> keys;
 const State& keysState(){return s;}
 const std::vector<Point2D_t>& keyList(){return keys;}
 unsigned char getKey(Point2D_t k){
  if(k.y==0 && k.x==0)return s.shift?'~':'`';
  if(k.y==0 && k.x==13) return 42;
  if(k.y==1 && k.x==0) return 43;
  if(k.y==2 && k.x==13) return 40;
  if(k.y==2 && k.x==1) return 129;
  if(k.y==2 && k.x==11) return s.shift?':':';';
  if(k.y==3 && k.x>=10 && k.x<=12) return ",./"[k.x-10];
  return k.x==2 ? (s.shift?'A':'a') : (s.shift?'B':'b');
 }
};
struct ButtonMock {bool wasPressed(){return false;}};
struct DeviceMock {KeyboardMock Keyboard; ButtonMock BtnA;} M5Cardputer;
