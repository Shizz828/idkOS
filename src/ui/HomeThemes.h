#pragma once
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "Screen.h"
#include "../core/AppRegistry.h"
#include "../core/BuildInfo.h"
#include "../core/ClockFormat.h"
#include "../core/HomeTheme.h"
#include "../core/Navigation.h"
#include "../services/WeatherData.h"
namespace spark {
// The three Home looks. They only draw: selection, launching and shortcuts stay in
// main.cpp and Navigation, shared by every theme. Everything is lines, boxes,
// circles and text, drawn again for each 8-row strip with no images or extra RAM.
struct HomeStation { uint16_t color; char letter; const char* name; bool darkLetter; };
// In HomeOrder: Cache, Weather, Crypto, Terminal, Calendar, Settings.
constexpr HomeStation HomeStations[HomeAppCount]={
    {theme::RouteCapture,'C',"CACHE",false},{theme::RouteWeather,'W',"WEATHER",false},
    {theme::RouteCrypto,'C',"CRYPTO",true},{theme::RouteTerminal,'T',"TERMINAL",true},
    {theme::RouteCalendar,'C',"CALENDAR",true},{theme::RouteSettings,'S',"SETTINGS",true}};
constexpr int HomeStationGap=38;
// Six centres 38 px apart on the 240 px screen: 25, 63, 101, 139, 177, 215.
inline int homeStationX(unsigned i){return 120+HomeStationGap*int(i)-HomeStationGap*int(HomeAppCount-1)/2;}
namespace homeColors {
constexpr uint16_t Muted=0x8410,Glow=0x0410,Navy=0x0010,Tile=0x4A49,Floor=0x2124,Rail=0x6B4D;
}
// Clock, date and weather text shared by the themes, built once per frame.
struct HomeInfo {
    bool clockSet=false,weatherValid=false,cached=false;
    int hour=-1,weatherCode=-1,degrees=0;
    char clock[12]{},utc[16]{},date[24]{},shortDate[24]{},temperature[8]{},condition[20]{},location[18]{};
};
inline HomeInfo homeInfo(time_t now,const WeatherSnapshot& w){
    HomeInfo info;
    if(now>=ClockMinimum){
        info.clockSet=true;struct tm local{},utc{};localtime_r(&now,&local);gmtime_r(&now,&utc);info.hour=local.tm_hour;
        formatClock(info.clock,sizeof info.clock,local.tm_hour,local.tm_min);
        char utcClock[9];formatClock(utcClock,sizeof utcClock,utc.tm_hour,utc.tm_min);snprintf(info.utc,sizeof info.utc,"UTC %s",utcClock);
        strftime(info.date,sizeof info.date,"%a %b %d %Y",&local);
        char day[4],month[4];strftime(day,sizeof day,"%a",&local);strftime(month,sizeof month,"%b",&local);
        snprintf(info.shortDate,sizeof info.shortDate,"%s %s %d, %d",day,month,local.tm_mday,local.tm_year+1900);
    } else { strcpy(info.clock,"--:--");strcpy(info.utc,"UTC --:--");strcpy(info.date,"Date: not set");strcpy(info.shortDate,"Date not set"); }
    if(w.weather.valid){
        info.weatherValid=true;info.weatherCode=w.weather.code;info.degrees=int(lroundf(w.weather.temperature));
        snprintf(info.temperature,sizeof info.temperature,"%dF",int(lroundf(w.weather.temperature)));
        strncpy(info.condition,condition(w.weather.code),sizeof info.condition-1);
        info.cached=!w.online || w.weatherFailed || stale(w.weather.updatedAt,now);
        memcpy(info.location,w.location.label,strnlen(w.location.label,sizeof info.location-1));
    }
    return info;
}
inline void clipText(char* out,size_t size,const char* value,size_t maxChars){
    size_t n=strlen(value);if(n>maxChars)n=maxChars;if(n>=size)n=size-1;memcpy(out,value,n);out[n]=0;
}
inline uint16_t stationInk(const HomeStation& s){return s.darkLetter?theme::Background:theme::Content;}

// ---- Theme 1: Subway (default) -------------------------------------------------
inline void drawSubwayTrain(int x,int y){
    auto& d=screen();d.fillRoundRect(x,y,9,10,2,theme::Primary);
    d.fillRect(x+2,y+2,5,3,homeColors::Navy);d.fillRect(x+2,y+7,1,1,homeColors::Navy);d.fillRect(x+6,y+7,1,1,homeColors::Navy);
}
inline void drawSubwayTunnel(){
    auto& d=screen();
    // Tiled side walls (brick pattern), left and right of the destination sign.
    for(int row=0;row<4;++row){
        const int y=88+row*6;
        d.drawFastHLine(0,y,72,homeColors::Tile);d.drawFastHLine(168,y,72,homeColors::Tile);
        for(int x=(row&1)*6;x<72;x+=12){d.drawFastVLine(x,y,6,homeColors::Tile);d.drawFastVLine(239-x,y,6,homeColors::Tile);}
    }
    // Ceiling lights angled toward the tunnel's vanishing point.
    for(int i=0;i<2;++i){
        const int x=4+i*30,y=110+i*4;
        d.drawLine(x,y-22,x+16,y-19,theme::Content);d.drawLine(x,y-21,x+16,y-18,theme::Content);
        d.drawLine(239-x,y-22,223-x,y-19,theme::Content);d.drawLine(239-x,y-21,223-x,y-18,theme::Content);
    }
    // Platforms with yellow safety edges, and the track between them.
    d.fillTriangle(0,134,0,125,86,113,homeColors::Floor);d.fillTriangle(0,134,86,113,86,134,homeColors::Floor);
    d.fillTriangle(239,134,239,125,153,113,homeColors::Floor);d.fillTriangle(239,134,153,113,153,134,homeColors::Floor);
    d.fillRect(87,113,66,22,theme::Background);
    d.drawLine(0,125,86,113,theme::Attention);d.drawLine(0,126,86,114,theme::Attention);
    d.drawLine(239,125,153,113,theme::Attention);d.drawLine(239,126,153,114,theme::Attention);
    d.drawLine(96,134,112,115,homeColors::Rail);d.drawLine(143,134,127,115,homeColors::Rail);
}
inline void drawSubwayHome(const HomeInfo& info,int selected,unsigned selectedApp){
    (void)selectedApp;auto& d=screen();d.fillScreen(theme::Background);
    text(6,2,"idkOS v1",1,theme::Primary);d.drawFastVLine(56,1,9,homeColors::Muted);text(62,2,"MAIN LINE",1,theme::Content);
    d.drawFastHLine(0,13,240,theme::Primary);d.drawFastHLine(0,14,240,homeColors::Glow);
    // Clock and date left, weather and place right.
    text(6,19,info.clock,2,theme::Content);text(6,38,info.date,1,homeColors::Muted);
    d.drawFastVLine(108,18,28,homeColors::Glow);
    char line[40];
    if(info.weatherValid){
        snprintf(line,sizeof line,"%s %s",info.temperature,info.condition);char shown[21];clipText(shown,sizeof shown,line,20);
        centerText(174,19,shown,1,info.cached?homeColors::Muted:theme::Content);
        centerText(174,29,info.location,1,theme::Primary);
        if(info.cached)centerText(174,39,"cached",1,homeColors::Muted);
    } else centerText(174,24,"Weather --",1,homeColors::Muted);
    // The route line and its six stations.
    constexpr int LineY=62,R=10;
    d.fillRect(0,LineY-1,240,3,theme::Primary);d.fillCircle(2,LineY,2,theme::Primary);d.fillCircle(237,LineY,2,theme::Primary);
    for(unsigned i=0;i<HomeAppCount;++i){
        const HomeStation& s=HomeStations[i];const int x=homeStationX(i);
        if(int(i)==selected){d.drawCircle(x,LineY,14,homeColors::Glow);d.drawCircle(x,LineY,13,theme::Primary);d.drawCircle(x,LineY,12,theme::Primary);}
        d.fillCircle(x,LineY,R,s.color);d.drawCircle(x,LineY,R,theme::Content);
        centerGlyph(x,LineY,s.letter,2,stationInk(s),s.color);
        smallText(x,78,s.name,int(i)==selected?theme::Primary:theme::Content);
    }
    // Tunnel and the destination sign showing the selected app.
    drawSubwayTunnel();
    if(selected<0)return;
    const int signW=140,signX=120-signW/2,signY=92,signH=19;
    d.fillRoundRect(signX,signY,signW,signH,4,homeColors::Navy);d.drawRoundRect(signX,signY,signW,signH,4,theme::Primary);
    drawSubwayTrain(signX+5,signY+5);drawSubwayTrain(signX+signW-14,signY+5);
    centerText(120,signY+2,HomeStations[selected].name,2,theme::Content,homeColors::Navy);
}

// ---- Theme 2: Metro Minimal ---------------------------------------------------
// Picture icons in Home order (Cache, Weather, Crypto, Terminal, Calendar, Settings),
// about 14 px across, drawn in `ink` on `paper`.
inline void drawMetroIcon(unsigned i,int x,int y,uint16_t ink,uint16_t paper){
    auto& d=screen();
    switch(i){
    case 0: // Cache: an envelope (notes and reminders).
        d.drawRect(x-7,y-5,15,11,ink);d.drawLine(x-7,y-5,x,y+1,ink);d.drawLine(x+7,y-5,x,y+1,ink);break;
    case 1: centerGlyph(x,y,'W',2,ink,paper);break;
    case 2: centerGlyph(x,y,'$',2,ink,paper);break;
    case 3: // Terminal: a ">_" prompt.
        for(int t=0;t<2;++t){d.drawLine(x-7+t,y-4,x-3+t,y,ink);d.drawLine(x-3+t,y,x-7+t,y+4,ink);}
        d.fillRect(x,y+3,7,2,ink);break;
    case 4: // Calendar: a page with rings and a date grid.
        d.drawRect(x-7,y-5,15,12,ink);d.fillRect(x-7,y-5,15,3,ink);d.fillRect(x-4,y-7,2,3,ink);d.fillRect(x+3,y-7,2,3,ink);
        for(int r=0;r<2;++r){for(int c=0;c<3;++c)d.fillRect(x-4+c*4,y+r*3,2,2,ink);}
        break;
    default:{ // Settings: a gear.
        static const int8_t teeth[8][2]={{0,-7},{5,-5},{7,0},{5,5},{0,7},{-5,5},{-7,0},{-5,-5}};
        for(const auto& t:teeth){d.fillRect(x+t[0]-1,y+t[1]-1,3,3,ink);}
        d.fillCircle(x,y,5,ink);d.fillCircle(x,y,2,paper);break;}
    }
}
// Weather symbol for the code (WMO), about 18 x 14: sun or moon, cloud, rain, snow, storm.
inline void drawWeatherIcon(int x,int y,int code,bool night){
    auto& d=screen();const uint16_t yellow=theme::Attention,cloud=theme::Content,blue=theme::Primary;
    const bool clear=code<=1,cloudy=code==2||code==3||code==45||code==48;
    if(clear || code==2){
        if(night){d.fillCircle(x,y,7,yellow);d.fillCircle(x+4,y-3,6,theme::Background);d.fillRect(x+6,y-7,1,3,yellow);d.fillRect(x+5,y-6,3,1,yellow);}
        else {d.fillCircle(x,y,5,yellow);for(int k=0;k<8;++k){static const int8_t r[8][2]={{0,-8},{6,-6},{8,0},{6,6},{0,8},{-6,6},{-8,0},{-6,-6}};d.fillRect(x+r[k][0]-1,y+r[k][1]-1,2,2,yellow);}}
        if(clear)return;
    }
    // Cloud (lower right when partly cloudy).
    const int cx=clear?x:(code==2?x+3:x),cy=code==2?y+3:y-1;
    d.fillCircle(cx-4,cy+1,4,cloud);d.fillCircle(cx+1,cy-2,5,cloud);d.fillCircle(cx+5,cy+1,4,cloud);d.fillRect(cx-4,cy+1,10,5,cloud);
    if(cloudy)return;
    const bool snow=(code>=71&&code<=77)||code==85||code==86,storm=code>=95;
    for(int k=0;k<3;++k){
        const int dx=cx-4+k*4;
        if(storm && k==1){d.drawLine(dx+1,cy+7,dx-1,cy+10,yellow);d.drawLine(dx-1,cy+10,dx+2,cy+10,yellow);d.drawLine(dx+2,cy+10,dx,cy+13,yellow);}
        else if(snow)d.fillRect(dx,cy+9,2,2,cloud);
        else d.drawLine(dx+1,cy+8,dx,cy+11,blue);
    }
}
inline void drawPin(int x,int y,uint16_t c){
    auto& d=screen();d.fillCircle(x,y+2,3,c);d.fillTriangle(x-3,y+3,x+3,y+3,x,y+8,c);d.fillCircle(x,y+2,1,theme::Background);
}
inline void drawMetroHome(const HomeInfo& info,int selected,unsigned selectedApp){
    (void)selectedApp;auto& d=screen();d.fillScreen(theme::Background);
    // "idkOS [V1]" and a cyan rule; Wi-Fi and battery come from the status bar.
    text(5,3,"idk",1,theme::Content);text(23,3,"OS",1,theme::Primary);
    d.drawRoundRect(40,1,17,11,2,theme::Primary);text(43,3,"V1",1,theme::Primary);
    d.drawFastHLine(0,14,240,theme::Primary);
    // Large clock; a 12-hour suffix goes small beside it.
    char clock[12];strcpy(clock,info.clock);char* suffix=strchr(clock,' ');if(suffix)*suffix++=0;
    auto& s=screen();s.setTextSize(3);const int clockW=s.textWidth(clock)-3;
    s.setTextSize(1);const int suffixW=suffix?s.textWidth(suffix)-1:0;
    const int left=62-(clockW+(suffix?suffixW+4:0))/2;
    text(left,20,clock,3,theme::Content);if(suffix)text(left+clockW+4,36,suffix,1,theme::Content);
    centerText(62,48,info.shortDate,1,theme::Content);
    d.drawFastVLine(124,19,40,homeColors::Muted);
    if(info.weatherValid){
        const uint16_t ink=info.cached?homeColors::Muted:theme::Content;
        drawWeatherIcon(144,30,info.weatherCode,info.hour>=0 && (info.hour<6 || info.hour>=18));
        // "52°F": number, a small degree ring, then F.
        char deg[8];snprintf(deg,sizeof deg,"%d",info.degrees);s.setTextSize(2);const int w=s.textWidth(deg);
        text(162,20,deg,2,ink);d.drawCircle(162+w+1,22,2,ink);text(162+w+5,20,"F",2,ink);
        char shown[14];clipText(shown,sizeof shown,info.cached?"cached":info.condition,13);text(162,38,shown,1,info.cached?homeColors::Muted:theme::Primary);
        drawPin(135,48,theme::Primary);char place[16];clipText(place,sizeof place,info.location,15);text(143,50,place,1,theme::Content);
    } else centerText(182,34,"Weather --",1,homeColors::Muted);
    // Six outlined icons; the selected one is filled and ringed in cyan.
    constexpr int RowY=83,R=12;
    for(unsigned i=0;i<HomeAppCount;++i){
        const HomeStation& st=HomeStations[i];const int x=homeStationX(i);const bool on=int(i)==selected;
        if(on){d.drawCircle(x,RowY,R+3,homeColors::Glow);d.drawCircle(x,RowY,R+2,theme::Primary);d.fillCircle(x,RowY,R,st.color);
            drawMetroIcon(i,x,RowY,stationInk(st),st.color);}
        else {d.drawCircle(x,RowY,R,st.color);d.drawCircle(x,RowY,R-1,st.color);drawMetroIcon(i,x,RowY,st.color,theme::Background);}
    }
    if(selected<0)return;
    const char* name=HomeStations[selected].name;s.setTextSize(1);
    const int boxW=s.textWidth(name)+19,boxX=120-boxW/2,boxY=106;
    d.drawRoundRect(boxX,boxY,boxW,14,5,theme::Primary);d.drawRoundRect(boxX+1,boxY+1,boxW-2,12,4,theme::Primary);
    centerText(120,boxY+3,name,1,theme::Content);
    d.fillRect(14,boxY+6,boxX-20,2,theme::Primary);d.fillRect(boxX+boxW+6,boxY+6,226-(boxX+boxW+6),2,theme::Primary);
}

// ---- Theme 3: Classic V1 ------------------------------------------------------
// The original Home layout, with letters and the app name centred by measurement.
inline void drawClassicHome(const HomeInfo& info,int selected,unsigned selectedApp){
    frame(ProductName);
    text(6,31,info.clock,2);text(150,34,info.utc);text(6,52,info.date);
    if(info.weatherValid){
        char glance[40];snprintf(glance,sizeof glance,"%s %s%s",info.temperature,info.condition,info.cached?" [cached]":"");glance[37]=0;
        text(6,66,glance);text(132,52,info.location,1,theme::Primary);
    } else text(6,66,"Weather: unavailable");
    constexpr int IconRowY=94;
    for(unsigned i=0;i<HomeAppCount;++i){
        const HomeStation& s=HomeStations[i];const int x=homeStationX(i);auto& d=screen();
        d.fillCircle(x,IconRowY,10,s.color);
        centerGlyph(x,IconRowY,s.letter,2,stationInk(s),s.color);
        if(int(i)==selected){d.drawCircle(x,IconRowY,13,theme::Primary);d.drawCircle(x,IconRowY,14,theme::Primary);}
    }
    if(selected>=0)centerText(120,116,Apps[selectedApp].label,1,HomeStations[selected].color);
}

inline void drawHomeTheme(HomeTheme theme,time_t now,const WeatherSnapshot& weather,unsigned selectedApp){
    const HomeInfo info=homeInfo(now,weather);const int selected=homePosition(selectedApp);
    if(theme==HomeTheme::MetroMinimal)drawMetroHome(info,selected,selectedApp);
    else if(theme==HomeTheme::Classic)drawClassicHome(info,selected,selectedApp);
    else drawSubwayHome(info,selected,selectedApp);
}
}
