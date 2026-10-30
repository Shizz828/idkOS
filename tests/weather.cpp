#include <cassert>
#include <cstring>
#include "../src/services/WeatherParser.h"
#include "../src/services/JsonDecode.h"
using namespace spark;
static const int64_t Now=1704067200;
static const char* fixture=R"json({
 "current_units":{"temperature_2m":"°F","wind_speed_10m":"mp/h"},
 "current":{"time":1704067200,"temperature_2m":52,"apparent_temperature":50,"relative_humidity_2m":68,"weather_code":3,"wind_speed_10m":8,"wind_direction_10m":270},
 "daily":{"temperature_2m_max":[56,58],"temperature_2m_min":[38,39]},
 "hourly":{"time":[1704067200,1704070800,1704074400,1704078000,1704081600],"temperature_2m":[52,55,52,46,44],"weather_code":[3,3,2,3,3]}
})json";
int main() {
 { // Forecast response is decoded only after TLS, from an unallocated document.
  DynamicJsonDocument filter(2048),deferred(0);
  filter["utc_offset_seconds"]=true;filter["current"]=true;filter["current_units"]=true;
  filter["daily"]["temperature_2m_max"]=true;filter["daily"]["temperature_2m_min"]=true;
  filter["hourly"]["time"]=true;filter["hourly"]["temperature_2m"]=true;filter["hourly"]["weather_code"]=true;
  assert(deferred.capacity()==0 && !filter.overflowed());
  assert(!decodeJson(deferred,fixture,filter.as<JsonVariantConst>(),16384));
  WeatherData forecast;assert(parseWeather(deferred.as<JsonVariantConst>(),Now,forecast));
  assert(forecast.valid && forecast.hours[3].valid && deferred.capacity()<=16384);
 }

 DynamicJsonDocument doc(8192); assert(!deserializeJson(doc,fixture));
 WeatherData w; assert(parseWeather(doc.as<JsonVariantConst>(),Now,w));
 assert(w.valid && w.dailyValid && w.temperature==52 && w.feelsLike==50);
 assert(w.hours[0].time==Now+3600 && w.hours[3].time==Now+14400);
 assert(!strcmp(windCompass(270),"W")); assert(!strcmp(windCompass(359),"N"));
 assert(!strcmp(windCompass(225),"SW")); assert(!strcmp(windCompass(90),"E"));
 for(int i=0;i<360;++i) assert(windCompass(i)[0]);
 assert(!stale(Now,Now+599)); assert(stale(Now,Now+600)); assert(stale(Now,0));
 doc["current"]["apparent_temperature"]=nullptr;
 assert(!parseWeather(doc.as<JsonVariantConst>(),Now,w)); assert(w.temperature==52);
 assert(!deserializeJson(doc,fixture));
 doc["hourly"]["temperature_2m"][1]=nullptr;
 assert(parseWeather(doc.as<JsonVariantConst>(),Now,w)); assert(!w.hours[0].valid);
 assert(!deserializeJson(doc,fixture));
 doc["hourly"]["time"][2]=Now+3600;
 assert(!parseWeather(doc.as<JsonVariantConst>(),Now,w));
 assert(!deserializeJson(doc,fixture)); doc["current_units"]["temperature_2m"]="°C";
 assert(!parseWeather(doc.as<JsonVariantConst>(),Now,w));
 assert(!deserializeJson(doc,fixture)); doc["current"]["relative_humidity_2m"]=101;
 assert(!parseWeather(doc.as<JsonVariantConst>(),Now,w));
 assert(!parseWeather(doc.as<JsonVariantConst>(),0,w));
 assert(isoEpoch("2024-01-01T00:00:00Z")==Now);
 assert(isoEpoch("2024-01-01T00:00:00-05:00")==Now+18000);
 assert(isoEpoch("2024-01-01T05:00:00+05:00")==Now);
 assert(!isoEpoch("2024-02-30T00:00:00Z")); assert(!isoEpoch("junk"));
 assert(!deserializeJson(doc,R"({"type":"FeatureCollection","features":[]})"));
 AlertData a; assert(parseAlerts(doc.as<JsonVariantConst>(),Now,a)); assert(a.known && a.count==0);
 WeatherSnapshot s; s.alerts=a; s.online=true; char text[64];
 alertSummary(s,Now,text,sizeof(text)); assert(!strcmp(text,"No alerts"));
 s.online=false; alertSummary(s,Now,text,sizeof(text)); assert(!strcmp(text,"Alerts: check stale"));
 s.online=true; s.alertsFailed=true; alertSummary(s,Now,text,sizeof(text)); assert(strcmp(text,"No alerts"));
 assert(!deserializeJson(doc,R"({"type":"FeatureCollection","features":[
 {"properties":{"event":"Wind Advisory","expires":"2024-01-01T02:00:00Z","severity":"Moderate"}},
 {"properties":{"event":"Flood Warning","expires":"2024-01-01T03:00:00Z","severity":"Severe"}},
 {"properties":{"event":"Old Alert","expires":"2024-01-01T00:00:00Z"}}
 ]})"));
 assert(parseAlerts(doc.as<JsonVariantConst>(),Now,a)); assert(a.count==2);
 assert(!strcmp(a.headline,"Flood Warning")); assert(a.earliestExpiry==Now+7200);
 doc.clear(); assert(!parseAlerts(doc.as<JsonVariantConst>(),Now,a)); assert(a.count==2);
 RefreshSchedule r; assert(r.ready(0)); r.finish(0,true);
 assert(!r.ready(599999)); assert(r.ready(600000)); r.finish(600000,false);
 assert(!r.ready(629999)); assert(r.ready(630000));
 RefreshSchedule wrap; wrap.finish(0xffffff00,false);
 assert(!wrap.ready(0xffffff01)); assert(wrap.ready(uint32_t(0xffffff00u+WeatherRetryMs)));
 { // Failure reasons separate no network, TLS/HTTPS, HTTP/API and bad data.
  WeatherSnapshot f; char reason[20];
  f.online=false; assert(weatherFaultText(f,reason,sizeof(reason)) && !strcmp(reason,"No network"));
  f.online=true; assert(!weatherFaultText(f,reason,sizeof(reason)));
  f.weatherFailed=true; f.weatherFault=WeatherFault::Https; f.weatherFaultCode=-32512;
  assert(weatherFaultText(f,reason,sizeof(reason)) && !strcmp(reason,"TLS error -32512"));
  f.weatherFaultCode=0; weatherFaultText(f,reason,sizeof(reason)); assert(!strcmp(reason,"HTTPS failed"));
  f.weatherFault=WeatherFault::Http; f.weatherFaultCode=503;
  weatherFaultText(f,reason,sizeof(reason)); assert(!strcmp(reason,"API HTTP 503"));
  f.weatherFault=WeatherFault::Data; weatherFaultText(f,reason,sizeof(reason)); assert(!strcmp(reason,"API data invalid"));
  f.weatherFault=WeatherFault::Memory; weatherFaultText(f,reason,sizeof(reason)); assert(!strcmp(reason,"Weather memory low"));
  // Cached weather stays visible and is labelled as cached after a failure.
  f.weather.valid=true; f.weather.updatedAt=Now; f.weather.utcOffset=0;
  weatherFreshness(f,Now+60,reason,sizeof(reason)); assert(!strncmp(reason,"Cached ",7));
 }
}
