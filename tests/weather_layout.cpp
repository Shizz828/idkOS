#include <initializer_list>
#include "../src/apps/WeatherHub.h"
int main() {
 for(bool twelve:{false,true}){spark::twelveHourClock()=twelve;

 spark::WeatherSnapshot s;
 spark::drawWeatherHub(s,0);
 s.online=true; s.weather.valid=true; s.weather.dailyValid=true;
 s.weather.updatedAt=1704067200; s.weather.high=56; s.weather.low=38;
 s.weather.feelsLike=50; s.weather.windSpeed=8; s.weather.windDirection=270;
 s.weather.humidity=68; s.weather.temperature=52;
 for(auto& h:s.weather.hours) { h.time=1704070800; h.temperature=55; h.valid=true; }
 for(int code: {0,1,2,3,45,48,51,56,61,66,71,77,80,85,95,99}) {
  s.weather.code=code; for(auto& h:s.weather.hours) h.code=code;
  spark::drawWeatherHub(s,1704067200);
 }
 s.weather.temperature=-150; s.weather.high=-140; s.weather.low=-150;
 s.weather.feelsLike=-180; s.weather.windSpeed=300;
 spark::drawWeatherHub(s,1704067200);
 s.weatherFailed=true; s.weatherFault=spark::WeatherFault::Https; s.weatherFaultCode=-32512;
 spark::drawWeatherHub(s,1704067200);
 s.weather.valid=false; spark::drawWeatherHub(s,1704067200); s.weather.valid=true;
 s.weatherFailed=false; s.weatherFault=spark::WeatherFault::None; s.weatherFaultCode=0;
 s.online=false; s.alerts.known=true; s.alerts.count=1;
 strcpy(s.alerts.headline,"A very long alert that must be clipped to the narrow status bar");
 spark::drawWeatherHub(s,1704067200);

 }
}
