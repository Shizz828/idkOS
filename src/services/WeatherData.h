#pragma once
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "../core/WifiProgress.h"
#include "../core/ClockFormat.h"
#include "../core/WeatherLocation.h"
namespace spark {
constexpr uint32_t WeatherRefreshMs=600000, WeatherRetryMs=30000;
constexpr int64_t ClockMinimum=1704067200;
struct WeatherHour { int64_t time=0; float temperature=0; int16_t code=-1; bool valid=false; };
struct WeatherData {
    bool valid=false, dailyValid=false;
    float temperature=0, feelsLike=0, high=0, low=0, windSpeed=0;
    int16_t code=-1, humidity=0, windDirection=0;
    int64_t updatedAt=0, observedAt=0;
    WeatherHour hours[4];
    int32_t utcOffset=INT32_MIN;
};
struct AlertData {
    bool known=false;
    uint16_t count=0;
    int64_t updatedAt=0, earliestExpiry=0;
    char headline[64]{};
};
// Why the last forecast request failed; offline is reported from `online`.
enum class WeatherFault : uint8_t { None, Https, Http, Data, Memory };
struct WeatherSnapshot {
    WifiProgress wifi;
    // Transient scan results only; credentials remain in KnownNetworks.
    struct NearbyNetwork { char ssid[33]{}; int rssi=0; };
    NearbyNetwork nearby[12];
    unsigned nearbyCount=0;
    uint32_t scanRevision=0;
    bool scanning=false,scanFailed=false;
    WeatherLocation location;
    LocationMode locationMode=LocationMode::Auto;
    unsigned knownNetworks=0;
    char knownSsids[5][33]{};
    bool networkSaveFailed=false;
    WeatherData weather;
    AlertData alerts;
    bool online=false, busy=false, fromDisk=false, cacheWriteFailed=false;
    bool weatherFailed=false, alertsFailed=false;
    WeatherFault weatherFault=WeatherFault::None;
    int16_t weatherFaultCode=0;
    char status[40]="Starting weather";
    uint32_t revision=0;
};
struct RefreshSchedule {
    bool scheduled=false;
    uint32_t due=0;
    bool ready(uint32_t now) const { return !scheduled || int32_t(now-due)>=0; }
    void finish(uint32_t now, bool success) {
        scheduled=true; due=now+(success ? WeatherRefreshMs : WeatherRetryMs);
    }
};
inline bool stale(int64_t updated, int64_t now) {
    return updated<ClockMinimum || now<ClockMinimum || now<updated || now-updated>=600;
}
inline const char* windCompass(int degrees) {
    static const char* names[]={"N","NNE","NE","ENE","E","ESE","SE","SSE","S","SSW","SW","WSW","W","WNW","NW","NNW"};
    return names[(((degrees%360+360)%360)*2+22)/45 %16];
}
inline const char* condition(int code) {
    switch(code) {
    case 0:return "Clear"; case 1:return "Mostly clear";
    case 2:return "Partly cloudy"; case 3:return "Overcast";
    case 45:case 48:return "Fog";
    case 51:case 53:case 55:return "Drizzle";
    case 56:case 57:return "Freezing drizzle";
    case 61:case 63:case 65:return "Rain";
    case 66:case 67:return "Freezing rain";
    case 71:case 73:case 75:case 77:return "Snow";
    case 80:case 81:case 82:return "Rain showers";
    case 85:case 86:return "Snow showers";
    case 95:case 96:case 99:return "Thunderstorm";
    default:return "Unknown";
    }
}
inline void weatherFreshness(const WeatherSnapshot& s,int64_t now,char* out,size_t capacity) {
    if(!s.weather.valid) { snprintf(out,capacity,"No weather yet"); return; }
    const char* label=!s.online?"Offline":s.fromDisk || s.weatherFailed || stale(s.weather.updatedAt,now)?"Cached":"Updated";
    time_t timestamp=s.weather.updatedAt; struct tm updated{};
    if(s.weather.utcOffset!=INT32_MIN) { timestamp+=s.weather.utcOffset; gmtime_r(&timestamp,&updated); }
    else localtime_r(&timestamp,&updated);
    char clock[9];formatClock(clock,sizeof(clock),updated.tm_hour,updated.tm_min);
    snprintf(out,capacity,"%s %s",label,clock);
}
// Short reason shown beside cached weather: no network, TLS/HTTPS, HTTP/API.
inline bool weatherFaultText(const WeatherSnapshot& s,char* out,size_t capacity) {
    if(!s.online) snprintf(out,capacity,"No network");
    else if(!s.weatherFailed) return false;
    else if(s.weatherFault==WeatherFault::Https) {
        if(s.weatherFaultCode) snprintf(out,capacity,"TLS error %d",s.weatherFaultCode);
        else snprintf(out,capacity,"HTTPS failed");
    } else if(s.weatherFault==WeatherFault::Http) snprintf(out,capacity,"API HTTP %d",s.weatherFaultCode);
    else if(s.weatherFault==WeatherFault::Memory) snprintf(out,capacity,"Weather memory low");
    else snprintf(out,capacity,"API data invalid");
    return true;
}
inline void alertSummary(const WeatherSnapshot& s, int64_t now, char* out, size_t size) {
    bool old=stale(s.alerts.updatedAt,now) || !s.online || s.alertsFailed;
    if(!s.alerts.known) snprintf(out,size,"Alerts unavailable");
    else if(s.alerts.count) {
        if(s.alerts.earliestExpiry && now>=s.alerts.earliestExpiry)
            snprintf(out,size,"Alerts expired; recheck needed");
        else snprintf(out,size,"%s%s",old ? "Cached: " : "",s.alerts.headline);
    } else snprintf(out,size,"%s",old ? "Alerts: check stale" : "No alerts");
}
}
