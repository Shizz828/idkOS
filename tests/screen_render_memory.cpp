#include <cassert>
#include "../src/ui/Screen.cpp"
using namespace spark;
int main(){
 assert(initScreenBuffer());assert(M5Canvas::binds==1 && M5Canvas::externalBytes==1920 && !M5Canvas::heapCreates);
 unsigned draws=0;
 for(int i=0;i<300;++i)renderScreen([&]{++draws;frame(i%2?"WEATHER":"SETTINGS");text(6,43,"Buffered app switch");});
 assert(draws==300*17 && M5Canvas::binds==1 && !M5Canvas::heapCreates);
 assert(M5Cardputer.Display.blits==300*17 && !M5Cardputer.Display.directWrites && !M5Cardputer.Display.dmaBusy);
 unsigned before=draws;renderScreenRegion(0,0,0,135,[&]{++draws;});assert(draws==before);
 renderScreenRegion(239,134,10,10,[&]{++draws;screen().fillScreen(theme::Primary);});assert(draws==before+1);
 assert(M5Cardputer.Display.pixels[134*240+239]==theme::Primary);
 assert(M5Canvas::externalBytes==1920 && !M5Canvas::heapCreates);
}
