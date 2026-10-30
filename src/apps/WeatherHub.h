#pragma once
#include "../services/WeatherData.h"
#include "../ui/Screen.h"
#include <time.h>
namespace spark {
inline void weatherIcon(int x, int y, int code, int scale=1) {
    auto& d=screen(); const uint16_t c=theme::Primary;
    if(code<0) { d.drawLine(x,y,x+12*scale,y,c); return; }
    if(code<=2) {
        d.drawCircle(x+7*scale,y+5*scale,3*scale,c);
        d.drawLine(x+7*scale,y,x+7*scale,y-2*scale,c);
        d.drawLine(x+7*scale,y+10*scale,x+7*scale,y+12*scale,c);
        d.drawLine(x,y+5*scale,x+2*scale,y+5*scale,c);
        d.drawLine(x+12*scale,y+5*scale,x+14*scale,y+5*scale,c);
        if(code==0 || code==1) return;
    }
    d.drawCircle(x+5*scale,y+6*scale,3*scale,c);
    d.drawCircle(x+10*scale,y+4*scale,4*scale,c);
    d.drawCircle(x+15*scale,y+7*scale,3*scale,c);
    d.drawLine(x+3*scale,y+10*scale,x+17*scale,y+10*scale,c);
    if(code==45 || code==48) {
        d.drawLine(x,y+13*scale,x+18*scale,y+13*scale,c); return;
    }
    if(code>=51 && code<=86) {
        for(int n=0;n<3;++n) {
            int a=x+(4+6*n)*scale, b=y+13*scale;
            d.drawLine(a,b,a-scale,b+3*scale,c);
            if((code>=71 && code<=77) || code>=85) d.drawLine(a-scale,b,a+scale,b+2*scale,c);
        }
    }
    if(code>=95) {
        d.drawLine(x+10*scale,y+12*scale,x+7*scale,y+16*scale,c);
        d.drawLine(x+7*scale,y+16*scale,x+11*scale,y+16*scale,c);
        d.drawLine(x+11*scale,y+16*scale,x+8*scale,y+20*scale,c);
    }
}
// A render-only function: its inputs are a copied cache snapshot and clock.
// No network, persistence, service updates or blocking waits occur here.
inline void drawWeatherHub(const WeatherSnapshot& s, int64_t now) {
    auto& d=screen();
    d.fillScreen(TFT_BLACK);
    weatherIcon(3,2,3);
    text(25,3,"WEATHER HUB",1,theme::Primary);
    text(103,3,"Enter:Wi-Fi settings",1,theme::Primary);
    char city[29]; snprintf(city,sizeof(city),"%s%s",s.location.approximate?"~":"",s.location.label);
    text(3,15,city,1,s.location.approximate?theme::Attention:theme::Content);
    char buffer[64];
    if(now>=ClockMinimum) {
        time_t t=now; struct tm utc{}; gmtime_r(&t,&utc);
        char utcClock[7];formatClock(utcClock,sizeof(utcClock),utc.tm_hour,utc.tm_min,true);
        snprintf(buffer,sizeof(buffer),"UTC %s",utcClock);
    } else snprintf(buffer,sizeof(buffer),"UTC --:--");
    text(177,15,buffer);
    d.drawFastHLine(0,25,240,theme::Primary);
    d.drawFastVLine(119,26,62,theme::Primary);
    if(s.weather.valid) {
        snprintf(buffer,sizeof(buffer),"%d",int(lroundf(s.weather.temperature)));
        text(4,31,buffer,4);
        int unitX=4+int(strlen(buffer))*24;
        d.drawCircle(unitX+2,33,1,theme::Primary);
        text(unitX+5,34,"F",1,theme::Primary);
        weatherIcon(79,43,s.weather.code);
        char age[20]; weatherFreshness(s,now,age,sizeof(age));
        char reason[20];
        bool faulted=weatherFaultText(s,reason,sizeof(reason));
        // A failed update keeps the cached forecast and adds one reason row.
        text(4,faulted?63:65,condition(s.weather.code));
        if(faulted) text(4,71,reason,1,theme::Error);
        text(4,faulted?80:78,age,1,s.weatherFailed?theme::Error:(!s.online || s.fromDisk || stale(s.weather.updatedAt,now))?theme::Attention:theme::Success);
        if(s.weather.dailyValid) snprintf(buffer,sizeof(buffer),"High: %dF",int(lroundf(s.weather.high)));
        else snprintf(buffer,sizeof(buffer),"High: --");
        text(125,30,buffer);
        if(s.weather.dailyValid) snprintf(buffer,sizeof(buffer),"Low: %dF",int(lroundf(s.weather.low)));
        else snprintf(buffer,sizeof(buffer),"Low: --");
        text(125,40,buffer);
        snprintf(buffer,sizeof(buffer),"Feels: %dF",int(lroundf(s.weather.feelsLike))); text(125,50,buffer);
        snprintf(buffer,sizeof(buffer),"Humidity: %d%%",s.weather.humidity); text(125,60,buffer);
        snprintf(buffer,sizeof(buffer),"Wind: %d mph",int(lroundf(s.weather.windSpeed))); text(125,70,buffer);
        text(125,80,windCompass(s.weather.windDirection),1,theme::Primary);
    } else {
        text(4,33,"--",4);
        text(4,69,"No weather yet");
        char status[19]{};
        if(!s.online) strlcpy(status,wifiMessage(s.wifi.phase),sizeof(status));
        else if(!weatherFaultText(s,status,sizeof(status))) strlcpy(status,s.status,sizeof(status));
        text(4,78,status,1,theme::Primary);
        text(125,30,"High: --"); text(125,40,"Low: --"); text(125,50,"Feels: --");
        text(125,60,"Humidity: --"); text(125,70,"Wind: --");
    }
    d.drawFastHLine(0,89,240,theme::Primary);
    text(3,92,"HOURLY - NEXT 4",1,theme::Primary);
    for(int i=0;i<4;++i) {
        int x=i*60+3; const auto& h=s.weather.hours[i];
        if(i) d.drawFastVLine(i*60,103,15,theme::Primary);
        if(h.time>=ClockMinimum) {
            time_t t=h.time; struct tm local{}; if(s.weather.utcOffset!=INT32_MIN) { t+=s.weather.utcOffset; gmtime_r(&t,&local); } else localtime_r(&t,&local);
            if(twelveHourClock())snprintf(buffer,sizeof(buffer),"%d%s",(local.tm_hour+11)%12+1,local.tm_hour>=12?"PM":"AM");
            else snprintf(buffer,sizeof(buffer),"%02dh",local.tm_hour);
        } else snprintf(buffer,sizeof(buffer),"--");
        text(x,103,buffer);
        weatherIcon(x+27,101,h.valid?h.code:-1);
        if(h.valid) snprintf(buffer,sizeof(buffer),"%dF",int(lroundf(h.temperature)));
        else snprintf(buffer,sizeof(buffer),"--");
        text(x,112,buffer);
    }
    d.drawFastHLine(0,121,240,theme::Primary);
    alertSummary(s,now,buffer,sizeof(buffer));
    // Keep one line: full alert details are intentionally outside this milestone.
    buffer[36]=0;
    d.fillTriangle(3,125,3,131,7,128,theme::Primary);
    text(11,125,buffer);
}
}
