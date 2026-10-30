#include <initializer_list>
#include "../src/ui/IdleBoardScreen.h"
int main() {
 for(bool twelve:{false,true}){spark::twelveHourClock()=twelve;
  spark::StatusState st;spark::WeatherSnapshot w;
  spark::drawIdleBoard(st,w,0,false);
  st.online=true;st.battery=100;st.clockMinute=23*60+59;
  w.weather.valid=true;w.weather.temperature=-150;w.weather.code=99;w.weather.updatedAt=1704067200;
  for(int battery:{-1,0,15,30,100,101}){st.battery=battery;spark::drawIdleBoard(st,w,1704067200,true);}
  st.online=false;spark::drawIdleBoard(st,w,1704067200,true);
  st.online=true;w.alerts.known=true;w.alerts.count=1;w.alerts.updatedAt=1704067200;
  spark::drawIdleBoard(st,w,1704067200,true);
  for(int minute:{0,9*60+5,12*60,13*60+45}){st.clockMinute=minute;spark::drawIdleBoard(st,w,1704067200,true);}
 }
}
