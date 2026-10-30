#pragma once
#include <ArduinoJson.h>
#include <string.h>
#include "WeatherData.h"
namespace spark {
inline bool numeric(JsonVariantConst v, float low, float high, float& out) {
    if(!v.is<float>()) return false;
    out=v.as<float>(); return isfinite(out) && out>=low && out<=high;
}
inline bool validCode(int code) { return strcmp(condition(code),"Unknown")!=0; }
inline bool parseWeather(JsonVariantConst root, int64_t now, WeatherData& out) {
    WeatherData next;
    auto current=root["current"];
    if(now<ClockMinimum || !current.is<JsonObjectConst>() ||
       strcmp(root["current_units"]["temperature_2m"] | "","°F") ||
       (strcmp(root["current_units"]["wind_speed_10m"] | "","mp/h") &&
        strcmp(root["current_units"]["wind_speed_10m"] | "","mph"))) return false;
    float value;
    if(!numeric(current["temperature_2m"],-150,160,next.temperature) ||
       !numeric(current["apparent_temperature"],-180,200,next.feelsLike) ||
       !numeric(current["wind_speed_10m"],0,300,next.windSpeed) ||
       !numeric(current["relative_humidity_2m"],0,100,value)) return false;
    next.humidity=int16_t(lroundf(value));
    if(!numeric(current["wind_direction_10m"],0,360,value)) return false;
    next.windDirection=int16_t(lroundf(value));
    if(!current["weather_code"].is<int>() || !validCode(current["weather_code"].as<int>())) return false;
    next.code=current["weather_code"].as<int>();
    if(!current["time"].is<int64_t>()) return false;
    next.observedAt=current["time"].as<int64_t>();
    // Reject implausible/future observations and unexpectedly old API responses.
    if(next.observedAt<ClockMinimum || next.observedAt>now+900 || now-next.observedAt>3600) return false;
    next.updatedAt=now;
    if(root["utc_offset_seconds"].is<int>() && root["utc_offset_seconds"].as<int>()>=-50400 && root["utc_offset_seconds"].as<int>()<=50400) next.utcOffset=root["utc_offset_seconds"].as<int>();
    float high,low;
    if(numeric(root["daily"]["temperature_2m_max"][0],-150,160,high) &&
       numeric(root["daily"]["temperature_2m_min"][0],-150,160,low) && high>=low) {
        next.high=high; next.low=low; next.dailyValid=true;
    }
    auto times=root["hourly"]["time"].as<JsonArrayConst>();
    auto temperatures=root["hourly"]["temperature_2m"].as<JsonArrayConst>();
    auto codes=root["hourly"]["weather_code"].as<JsonArrayConst>();
    if(times.isNull() || times.size()!=temperatures.size() || times.size()!=codes.size() || times.size()>72) return false;
    int n=0; int64_t last=0;
    for(size_t i=0;i<times.size() && n<4;++i) {
        if(!times[i].is<int64_t>()) return false;
        int64_t epoch=times[i].as<int64_t>();
        if(epoch<=last) return false;
        last=epoch;
        if(epoch<=now) continue;
        auto& hour=next.hours[n++]; hour.time=epoch;
        if(numeric(temperatures[i],-150,160,hour.temperature) && codes[i].is<int>() && validCode(codes[i].as<int>())) {
            hour.code=codes[i].as<int>(); hour.valid=true;
        }
    }
    if(n!=4) return false;
    next.valid=true; out=next; return true;
}
// NWS ISO8601 timestamps include Z or an explicit numeric UTC offset.
inline int64_t isoEpoch(const char* s) {
    if(!s) return 0;
    int y,m,d,h,min,sec,used=0;
    if(sscanf(s,"%4d-%2d-%2dT%2d:%2d:%2d%n",&y,&m,&d,&h,&min,&sec,&used)!=6 || used!=19 ||
       y<2024 || y>2100 || m<1 || m>12 || h<0 || h>23 || min<0 || min>59 || sec<0 || sec>59) return 0;
    const int monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31};
    int days=monthDays[m-1]+(m==2 && (y%4==0 && (y%100!=0 || y%400==0)));
    if(d<1 || d>days) return 0;
    int offset=0;
    if(s[19]=='Z' && s[20]==0) {}
    else if((s[19]=='+' || s[19]=='-') && strlen(s)==25) {
        int oh,om,n=0;
        if(sscanf(s+20,"%2d:%2d%n",&oh,&om,&n)!=2 || n!=5 || oh<0 || oh>23 || om<0 || om>59) return 0;
        offset=(oh*60+om)*60*(s[19]=='+'?1:-1);
    } else return 0;
    // Gregorian civil date to Unix days.
    y-=m<=2; int era=y/400; unsigned yy=y-era*400;
    unsigned day=(153*(m+(m>2?-3:9))+2)/5+d-1;
    int64_t total=era*146097+(yy*365+yy/4-yy/100+day)-719468;
    return total*86400+h*3600+min*60+sec-offset;
}
inline void compactText(const char* input, char* output, size_t size) {
    size_t n=0;
    for(const unsigned char* p=reinterpret_cast<const unsigned char*>(input); *p && n+1<size; ++p)
        if(*p>=32 && *p<127) output[n++]=char(*p);
    output[n]=0;
}
inline bool parseAlerts(JsonVariantConst root, int64_t now, AlertData& out) {
    if(strcmp(root["type"] | "","FeatureCollection") || !root["features"].is<JsonArrayConst>() || now<ClockMinimum) return false;
    AlertData next; next.known=true; next.updatedAt=now;
    int chosen=-1;
    for(auto feature:root["features"].as<JsonArrayConst>()) {
        auto p=feature["properties"];
        if(!p.is<JsonObjectConst>() || !p["event"].is<const char*>()) return false;
        const char* status=p["status"] | "Actual";
        if(strcmp(status,"Actual")) continue;
        if(!strcmp(p["messageType"] | "","Cancel")) continue;
        const char* expiry=p["expires"] | "";
        int64_t expires=isoEpoch(expiry);
        if(*expiry && !expires) return false;
        if(expires && expires<=now) continue;
        int64_t effective=isoEpoch(p["effective"] | "");
        if(effective>now) continue;
        if(expires && (!next.earliestExpiry || expires<next.earliestExpiry)) next.earliestExpiry=expires;
        if(next.count<65535) ++next.count;
        const char* severity=p["severity"] | "Unknown";
        int priority=!strcmp(severity,"Extreme")?4:!strcmp(severity,"Severe")?3:!strcmp(severity,"Moderate")?2:1;
        if(priority>chosen) { chosen=priority; compactText(p["event"].as<const char*>(),next.headline,sizeof(next.headline)); }
    }
    if(next.count && !next.headline[0]) return false;
    out=next; return true;
}
}
