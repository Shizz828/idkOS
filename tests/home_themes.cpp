#include <cassert>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include "../src/ui/HomeThemes.h"
#include "../src/apps/SettingsApp.h"
// The three Home themes on the 240x135 panel (the fake asserts every element stays
// on screen), centring, the Calendar station, the saved choice and the Settings page.
using namespace spark;
struct Store {
    std::map<std::string,uint8_t> values;bool fail=false;
    uint8_t getUChar(const char* k,uint8_t d){auto it=values.find(k);return it==values.end()?d:it->second;}
    size_t putUChar(const char* k,uint8_t v){if(fail)return 0;values[k]=v;return 1;}
};
struct Network { bool networkAction(uint8_t,unsigned=0){return true;} };
static std::vector<FakeDisplay::Printed>& printed(){return M5Cardputer.Display.printed;}
static bool shows(const char* t){for(const auto& p:printed())if(p.text==t)return true;return false;}
static bool showsPart(const char* t){for(const auto& p:printed())if(p.text.find(t)!=std::string::npos)return true;return false;}
static const FakeDisplay::Printed* find(const char* t,int nth=0){for(const auto& p:printed())if(p.text==t && nth--==0)return &p;return nullptr;}
int main(){
    setenv("TZ","UTC0",1);tzset();
    const time_t nov10=1794355260;   // 2026-11-10 23:41 UTC
    WeatherSnapshot w;w.weather.valid=true;w.weather.temperature=52;w.weather.code=0;w.online=true;
    w.weather.updatedAt=nov10-60;strncpy(w.location.label,"Newark, NJ",sizeof w.location.label-1);
    WeatherSnapshot longW=w;longW.weather.code=56;longW.online=false;   // "Freezing drizzle", cached
    memset(longW.location.label,'X',sizeof longW.location.label-1);longW.location.label[sizeof longW.location.label-1]=0;
    WeatherSnapshot none;
    const HomeTheme themes[]={HomeTheme::Subway,HomeTheme::MetroMinimal,HomeTheme::Classic};
    // Every theme, every selection (SSH has no station), clock set or not, 12/24-hour, any weather.
    for(HomeTheme t:themes)for(unsigned app=0;app<AppCount;++app)for(int clock=0;clock<2;++clock)for(int twelve=0;twelve<2;++twelve){
        twelveHourClock()=twelve;
        for(const WeatherSnapshot* ws:{&w,&longW,&none}){
            drawHomeTheme(t,clock?nov10:0,*ws,app);
            assert(!showsPart("MEDIA") && !showsPart("Media"));
            const int pos=homePosition(app);
            if(pos>=0 && t!=HomeTheme::Classic)assert(shows(HomeStations[pos].name));
        }
    }
    twelveHourClock()=false;
    // Station letters: C W C T C S, the orange one is Calendar; each centred in its circle.
    const char letters[]="CWCTCS";
    assert(HomeOrder[4]==CalendarIndex && HomeStations[4].color==theme::RouteCalendar && HomeStations[4].letter=='C' && !strcmp(HomeStations[4].name,"CALENDAR"));
    assert(Apps[CalendarIndex].badge=='C');
    const int rows[]={62,83,94};
    for(int ti=0;ti<3;++ti){
        drawHomeTheme(themes[ti],nov10,w,CalendarIndex);
        int seen[128]={};
        for(unsigned i=0;i<HomeAppCount;++i){
            if(themes[ti]==HomeTheme::MetroMinimal && i!=1)continue;   // Metro uses picture icons except W.
            const char l[2]={letters[i],0};const auto* p=find(l,seen[int(letters[i])]++);assert(p);
            // A size-2 glyph is 10x14 px; its centre lands within half a pixel of the circle's.
            assert(p->x==homeStationX(i)-5 && p->y==rows[ti]-7);
        }
        // The selected name is centred on the screen.
        const char* name=themes[ti]==HomeTheme::Classic?"CALENDAR":"CALENDAR";
        int n=0;for(const auto& p:printed())if(p.text==name){++n;}
        assert(n>=1);
    }
    // Subway sign (size 2) and Metro box (size 1) centre the selected name on x=120.
    drawHomeTheme(HomeTheme::Subway,nov10,w,WeatherIndex);{const auto* p=find("WEATHER",1);assert(p && p->x==120-(84-2)/2);}
    drawHomeTheme(HomeTheme::MetroMinimal,nov10,w,WeatherIndex);{const auto* p=find("WEATHER");assert(p && p->x==120-(42-1)/2);}
    drawHomeTheme(HomeTheme::Classic,nov10,w,WeatherIndex);{const auto* p=find("WEATHER HUB");assert(p && p->x==120-(66-1)/2);}
    // Saved choice: missing or unknown means Subway; saving applies it at once.
    Store s;homeTheme()=HomeTheme::Classic;loadHomeTheme(s);assert(homeTheme()==HomeTheme::Subway);
    s.values["theme"]=9;loadHomeTheme(s);assert(homeTheme()==HomeTheme::Subway);
    assert(saveHomeTheme(s,HomeTheme::MetroMinimal) && s.values["theme"]==1);
    homeTheme()=HomeTheme::Subway;loadHomeTheme(s);assert(homeTheme()==HomeTheme::MetroMinimal);
    s.fail=true;assert(!saveHomeTheme(s,HomeTheme::Classic) && homeTheme()==HomeTheme::Classic);
    // Settings > Display > H: list starts on the current theme, arrows wrap, Enter saves, Esc returns to Display.
    SettingsApp settings;Network net;SystemSnapshot state;InputEvent e;
    homeTheme()=HomeTheme::Subway;settings.page=SettingsApp::Display;settings.draw(w,state);assert(shows("H Home theme: Subway"));
    strcpy(e.text,"h");assert(settings.input(e,w,net)==SettingsAction::None && settings.page==SettingsApp::HomeThemePage && settings.themeSelected==0);
    e=InputEvent();e.up=true;settings.input(e,w,net);assert(settings.themeSelected==2);
    e=InputEvent();e.down=true;settings.input(e,w,net);settings.input(e,w,net);assert(settings.themeSelected==1);
    settings.draw(w,state);assert(shows("1 Subway  (current)") && shows("2 Metro Minimal") && shows("3 Classic V1"));
    e=InputEvent();e.enter=true;assert(settings.input(e,w,net)==SettingsAction::SaveHomeTheme);
    settings.savedHomeTheme(true);homeTheme()=HomeTheme::MetroMinimal;settings.draw(w,state);assert(shows("2 Metro Minimal  (current)") && shows("Home theme saved"));
    assert(settings.back() && settings.page==SettingsApp::Display && settings.back() && settings.page==SettingsApp::Main);
    return 0;
}
