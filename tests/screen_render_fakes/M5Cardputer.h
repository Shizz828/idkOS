#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
constexpr uint16_t TFT_BLACK=0,TFT_WHITE=65535;
struct FakeDisplay {
 std::array<uint16_t,240*135> pixels{};
 int width=240,height=135,left=0,top=0,clipWidth=240,clipHeight=135;
 int cursorX=0,cursorY=0,textSize=1;
 uint16_t foreground=TFT_WHITE,background=TFT_BLACK;
 bool panel=false,dmaBusy=false;
 FakeDisplay* sourceParent=nullptr;
 unsigned directWrites=0,blits=0,dmaWaits=0;
 std::array<unsigned,135> rowsTransferred{};
 void put(int x,int y,uint16_t color,bool transfer=false){
  if(sourceParent)assert(!sourceParent->dmaBusy);
  if(x<0||y<0||x>=width||y>=height||x<left||y<top||x>=left+clipWidth||y>=top+clipHeight)return;
  pixels[y*240+x]=color;if(panel&&!transfer)++directWrites;
 }
 void waitDMA(){dmaBusy=false;++dmaWaits;}
 void setClipRect(int x,int y,int w,int h){left=x;top=y;clipWidth=w;clipHeight=h;}
 void clearClipRect(){setClipRect(0,0,width,height);}
 void fillScreen(uint16_t c){fillRect(0,0,width,height,c);}
 void fillRect(int x,int y,int w,int h,uint16_t c){
  for(int row=std::max(0,std::max(y,top));row<std::min(height,std::min(y+h,top+clipHeight));++row)
   for(int col=std::max(0,std::max(x,left));col<std::min(width,std::min(x+w,left+clipWidth));++col)put(col,row,c);
 }
 void drawFastHLine(int x,int y,int w,uint16_t c){fillRect(x,y,w,1,c);}
 void drawFastVLine(int x,int y,int h,uint16_t c){fillRect(x,y,1,h,c);}
 void drawLine(int x,int y,int x2,int y2,uint16_t c){
  int dx=std::abs(x2-x),sx=x<x2?1:-1,dy=-std::abs(y2-y),sy=y<y2?1:-1,err=dx+dy;
  for(;;){put(x,y,c);if(x==x2&&y==y2)break;int twice=2*err;if(twice>=dy){err+=dy;x+=sx;}if(twice<=dx){err+=dx;y+=sy;}}
 }
 void fillCircle(int x,int y,int r,uint16_t c){for(int j=-r;j<=r;++j)for(int i=-r;i<=r;++i)if(i*i+j*j<=r*r)put(x+i,y+j,c);}
 void drawCircle(int x,int y,int r,uint16_t c){for(int j=-r;j<=r;++j)for(int i=-r;i<=r;++i)if(std::abs(i*i+j*j-r*r)<=r)put(x+i,y+j,c);}
 void fillTriangle(int ax,int ay,int bx,int by,int cx,int cy,uint16_t c){
  auto edge=[](int x,int y,int a,int b,int u,int v){return (x-a)*(v-b)-(y-b)*(u-a);};
  for(int y=std::min(ay,std::min(by,cy));y<=std::max(ay,std::max(by,cy));++y)
   for(int x=std::min(ax,std::min(bx,cx));x<=std::max(ax,std::max(bx,cx));++x){
    int a=edge(x,y,ax,ay,bx,by),b=edge(x,y,bx,by,cx,cy),d=edge(x,y,cx,cy,ax,ay);
    if((a>=0&&b>=0&&d>=0)||(a<=0&&b<=0&&d<=0))put(x,y,c);
   }
 }
 void setTextSize(int s){textSize=s;}
 void setTextColor(uint16_t fg,uint16_t bg){foreground=fg;background=bg;}
 void setCursor(int x,int y){cursorX=x;cursorY=y;}
 void print(const char* text){
  for(const unsigned char* p=(const unsigned char*)text;*p;++p){
   for(int y=0;y<8;++y)for(int x=0;x<6;++x){
    bool mark=*p!=' '&&x<5&&y<7&&(((*p>>(x%5))^(y+1))&1);
    fillRect(cursorX+x*textSize,cursorY+y*textSize,textSize,textSize,mark?foreground:background);
   }
   cursorX+=6*textSize;
  }
 }
};
namespace lgfx {using LGFXBase=FakeDisplay;}
struct FakeDevice {FakeDisplay Display;FakeDevice(){Display.panel=true;}} M5Cardputer;
class M5Canvas : public FakeDisplay {
 FakeDisplay* parent;
public:
 static unsigned binds,heapCreates,externalBytes;
 explicit M5Canvas(FakeDisplay* p):parent(p){sourceParent=p;}
 void setPsram(bool enabled){assert(!enabled);}
 void setColorDepth(int bits){assert(bits==8);}
 void setBuffer(void* storage,int w,int h){assert(storage&&uintptr_t(storage)%4==0);width=w;height=h;clearClipRect();++binds;externalBytes=w*h;}
 void setTextWrap(bool){}
 void* createSprite(int,int){++heapCreates;assert(!"renderer must use fixed storage");return nullptr;}
 void pushSprite(int x,int y){
  assert(!parent->dmaBusy);++parent->blits;
  for(int row=0;row<height;++row){
   if(y+row<parent->top||y+row>=parent->top+parent->clipHeight||y+row<0||y+row>=parent->height)continue;
   ++parent->rowsTransferred[y+row];
   for(int col=0;col<width;++col)parent->put(x+col,y+row,pixels[row*240+col],true);
  }
  parent->dmaBusy=true;
 }
};
unsigned M5Canvas::binds=0;unsigned M5Canvas::heapCreates=0;unsigned M5Canvas::externalBytes=0;
