#pragma once
#include "Screen.h"
#include "../core/BuildInfo.h"
#include "StatusBar.h"
#include "../services/WeatherData.h"
#include <time.h>
namespace spark {
inline uint16_t idleBatteryColor(int battery) {
    if(battery<0 || battery>100)return theme::Attention;
    return battery<=15?theme::Error:battery<=30?theme::Attention:theme::Success;
}
inline void drawIdleBoard(const StatusState& status,const WeatherSnapshot& weather,
                          int64_t epoch,bool timeReady) {
    auto& d=screen();d.fillScreen(theme::Background);
    text(6,5,ProductName,1,theme::Content);text(150,5,"DEPARTURES",1,theme::RouteTerminal);
    d.drawFastHLine(6,18,228,theme::Primary);
    char clock[7];formatClockMinute(clock,sizeof(clock),timeReady?status.clockMinute:-1,true);
    // Static split-flap cells: no animation task or per-second repaint.
    for(unsigned i=0;clock[i];++i){
        int x=6+23*int(i);char ch[]={clock[i],0};
        d.drawFastHLine(x,26,20,theme::RouteSettings);d.drawFastHLine(x,56,20,theme::RouteSettings);
        d.drawFastVLine(x,26,31,theme::RouteSettings);d.drawFastVLine(x+19,26,31,theme::RouteSettings);
        text(x+4,32,ch,2,timeReady?theme::Content:theme::Attention);
        d.drawFastHLine(x+1,43,18,theme::RouteSettings);
    }
    if(timeReady && epoch>=ClockMinimum){
        time_t now=epoch;struct tm local{};localtime_r(&now,&local);char date[16];
        strftime(date,sizeof(date),"%a",&local);text(155,27,date);
        strftime(date,sizeof(date),"%b %d",&local);text(155,40,date);
        strftime(date,sizeof(date),"%Y",&local);text(155,53,date);
    }else {text(155,31,"DATE",1,theme::RouteSettings);text(155,44,"NOT SET",1,theme::Attention);}
    text(6,70,"WEATHER",1,theme::RouteSettings);
    char summary[28];uint16_t weatherColor=theme::Attention;
    if(weather.weather.valid && isfinite(weather.weather.temperature)){
        snprintf(summary,sizeof(summary),"%dF %.18s",int(lroundf(weather.weather.temperature)),condition(weather.weather.code));
        bool cached=!status.online || weather.weatherFailed || stale(weather.weather.updatedAt,epoch);
        weatherColor=cached?theme::Attention:theme::Success;
        if(cached)snprintf(summary,sizeof(summary),"%dF CACHED",int(lroundf(weather.weather.temperature)));
    }else snprintf(summary,sizeof(summary),"UNAVAILABLE");
    if(weather.alerts.known && weather.alerts.count && !weather.alertsFailed && status.online &&
       !stale(weather.alerts.updatedAt,epoch) && (!weather.alerts.earliestExpiry || epoch<weather.alerts.earliestExpiry)){
        snprintf(summary,sizeof(summary),"WEATHER ALERT");weatherColor=theme::Error;
    }
    text(66,70,summary,1,weatherColor);
    text(6,87,"NETWORK",1,theme::RouteSettings);
    text(66,87,status.online?"ONLINE":"OFFLINE",1,status.online?theme::Success:theme::Attention);
    char battery[12];
    if(status.battery>=0 && status.battery<=100)snprintf(battery,sizeof(battery),"BAT %d%%",status.battery);
    else snprintf(battery,sizeof(battery),"BAT --%%");
    text(156,87,battery,1,idleBatteryColor(status.battery));
    d.drawFastHLine(6,102,228,theme::RouteSettings);
    text(6,108,"IDLE",1,theme::Content);text(102,108,"Esc/Enter: back");
    text(6,123,"Fn+Esc sleep",1,theme::Attention);
}
}
