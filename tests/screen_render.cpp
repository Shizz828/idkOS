#include <cassert>
#include "../src/ui/Screen.cpp"
#include "../src/apps/WeatherHub.h"
#include "../src/ui/StatusBar.h"
using namespace spark;
template<typename Draw>void equivalent(const Draw& draw){
 FakeDisplay expected;auto* saved=screenTarget();screenTarget()=&expected;screen().offset(0);draw();screenTarget()=saved;
 M5Cardputer.Display.directWrites=0;M5Cardputer.Display.rowsTransferred.fill(0);unsigned before=M5Cardputer.Display.blits;
 renderScreen(draw);
 assert(M5Cardputer.Display.pixels==expected.pixels);
 assert(!M5Cardputer.Display.directWrites && !M5Cardputer.Display.dmaBusy);
 assert(M5Cardputer.Display.blits-before==17);
 for(auto rows:M5Cardputer.Display.rowsTransferred)assert(rows==1); // Includes bottom partial strip.
}
int main(){
 assert(initScreenBuffer());WeatherSnapshot state;StatusState status;status.online=true;status.battery=30;status.clockMinute=1327;
 equivalent([&]{drawWeatherHub(state,0);drawStatus(status);});
 state.online=true;state.weather.valid=state.weather.dailyValid=true;state.weather.updatedAt=1704067200;
 state.weather.temperature=52;state.weather.high=56;state.weather.low=38;state.weather.humidity=68;
 for(auto& h:state.weather.hours){h.valid=true;h.time=1704070800;h.temperature=55;}
 for(int code:{0,2,3,45,61,71,95}){state.weather.code=code;for(auto& h:state.weather.hours)h.code=code;equivalent([&]{drawWeatherHub(state,1704067200);drawStatus(status);});}
 equivalent([&]{frame("SETTINGS");text(6,34,"> Wi-Fi",2);screen().fillCircle(120,80,10,theme::Success);screen().drawLine(0,0,239,134,theme::Primary);text(6,123,"Fn switching / bottom row");drawStatus(status);});
 // A status-only update must leave all app pixels outside its region intact.
 auto previous=M5Cardputer.Display.pixels;status.battery=12;status.clockMinute=1328;
 renderScreenRegion(142,0,98,12,[&]{drawStatus(status);});
 for(int y=0;y<135;++y)for(int x=0;x<240;++x)if(x<142||y>=12)assert(M5Cardputer.Display.pixels[y*240+x]==previous[y*240+x]);
 // A fresh Weather frame after an app switch must completely replace the prior view.
 equivalent([&]{drawWeatherHub(state,1704067200);drawStatus(status);});
}
