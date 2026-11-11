#pragma once
#include "../core/Input.h"
#include "../core/BuildInfo.h"
#include "../core/ClockFormat.h"
#include "../services/WeatherData.h"
#include "../terminal/TerminalData.h"
#include "../ui/Screen.h"
#include "../core/HomeTheme.h"
namespace spark {
struct SystemSnapshot {
    uint32_t uptime=0,heap=0,minimumHeap=0; int battery=-1,rssi=0;bool online=false,timeReady=false;
    char ssid[33]{},ip[20]{};StorageStats storage;
};
enum class SettingsAction { None,AddWifi,Location,Keyboard,Restart,SaveBrightness,Developer,SaveClockFormat,SaveHomeTheme };
class SettingsApp {
    unsigned selected=0,networkSelected=0,maintenancePage=0;
    bool forgetConfirm=false,restartConfirm=false;
    char forgetName[33]{};
    const char* message="";
public:
    // HomeTheme is reached from Display (H) and returns there.
    enum Page { Main,Wifi,Display,Maintenance,Shortcuts,About,HomeThemePage } page=Main;
    int brightness=100;
    unsigned themeSelected=0;
    void openWifi(){page=Wifi;forgetConfirm=restartConfirm=false;message="";}
    bool back() {
        if(forgetConfirm || restartConfirm){forgetConfirm=restartConfirm=false;return true;}
        if(page==HomeThemePage){page=Display;message="";return true;}
        if(page!=Main){page=Main;message="";return true;}return false;
    }
    void savedBrightness(bool ok){message=ok?"Brightness saved":"Settings save failed";}
    void savedClockFormat(bool ok){message=ok?"Clock format saved":"Settings save failed";}
    void savedHomeTheme(bool ok){message=ok?"Home theme saved":"Theme applied; save failed";}
    void cancelRestart(){restartConfirm=false;}
    void storageBusy(){message="SD busy; retry restart";}
    template<typename Service> SettingsAction input(const InputEvent& e,const WeatherSnapshot& weather,Service& service) {
        if(restartConfirm){if(e.text[0]=='y'||e.text[0]=='Y'){restartConfirm=false;return SettingsAction::Restart;}if(e.enter||e.text[0])restartConfirm=false;return SettingsAction::None;}
        if(forgetConfirm){if(e.text[0]=='y'||e.text[0]=='Y') {if(networkSelected>=weather.knownNetworks || strcmp(forgetName,weather.knownSsids[networkSelected]))message="Network list changed; select again";else message=service.networkAction(2,networkSelected)?"Forget requested":"Network worker busy";}if(e.enter||e.text[0])forgetConfirm=false;return SettingsAction::None;}
        char c=e.text[0]; if(c>='A'&&c<='Z')c+=32;
        if(page==Main) {
            if(e.up) selected=(selected+4)%5;
            if(e.down) selected=(selected+1)%5;
            if(c>='1'&&c<='5'){selected=c-'1';page=Page(selected+1);message="";}
            if(e.enter){page=Page(selected+1);message="";}
            if(c=='d')return SettingsAction::Developer;
            if(c=='w')page=Wifi;
            if(c=='l')return SettingsAction::Location;
            if(c=='k')return SettingsAction::Keyboard;
        } else if(page==Wifi) {
            if(networkSelected>=weather.knownNetworks)networkSelected=weather.knownNetworks?weather.knownNetworks-1:0;
            if(e.up && networkSelected)--networkSelected;
            if(e.down && networkSelected+1<weather.knownNetworks)++networkSelected;
            if(c=='a'||c=='s')return SettingsAction::AddWifi;
            if(c=='r')message=service.networkAction(3)?"Reconnect requested":"Network worker busy";
            if(c=='d'&&weather.knownNetworks){forgetConfirm=true;strlcpy(forgetName,weather.knownSsids[networkSelected],sizeof(forgetName));}
            if(e.enter&&weather.knownNetworks)message=service.networkAction(1,networkSelected)?"Connect requested":"Network worker busy";
        } else if(page==Display) {
            if(e.left||e.down)brightness=brightness>30?brightness-10:20;
            if(e.right||e.up)brightness=brightness<245?brightness+10:255;
            if(c=='t')return SettingsAction::SaveClockFormat;
            if(c=='h'){page=HomeThemePage;themeSelected=unsigned(homeTheme());message="";return SettingsAction::None;}
            if(e.enter)return SettingsAction::SaveBrightness;
        } else if(page==HomeThemePage) {
            if(e.up)themeSelected=(themeSelected+HomeThemeCount-1)%HomeThemeCount;
            if(e.down)themeSelected=(themeSelected+1)%HomeThemeCount;
            if(c>='1'&&c<='3')themeSelected=unsigned(c-'1');
            if(e.enter)return SettingsAction::SaveHomeTheme;
        } else if(page==Maintenance) {
            if(e.left||e.right)maintenancePage=1-maintenancePage;
            if(c=='r')restartConfirm=true;
            if(c=='w')message=service.networkAction(3)?"Reconnect requested":"Network worker busy";
        }
        return SettingsAction::None;
    }
    void draw(const WeatherSnapshot& weather,const SystemSnapshot& state) const {
        if(forgetConfirm || restartConfirm) {
            frame("CONFIRM");char restart[24];snprintf(restart,sizeof(restart),"Restart %s?",ProductName);text(6,37,forgetConfirm?"Forget selected Wi-Fi network?":restart,1,theme::Attention);
            if(forgetConfirm)text(6,55,forgetName,1,theme::Error);
            text(6,77,restartConfirm?"Save notes before restarting":"Saved password will be removed",1,theme::Error);
            text(6,103,"Y confirms; Enter/other = No",1,theme::Attention);return;
        }
        if(page==Main){
            frame("SETTINGS");char about[24];snprintf(about,sizeof(about),"5 About %s",ProductName);const char* labels[]={"1 Wi-Fi","2 Display","3 System Maintenance","4 Shortcut List",about};
            for(unsigned i=0;i<5;++i){text(6,32+int(i)*15,i==selected?">":" ",1,theme::Primary);text(18,32+int(i)*15,labels[i],1,i==selected?theme::Primary:theme::Content);}
            text(6,111,"D Developer Mode (SYS read-only)",1,theme::Primary);text(6,126,"L location  K keyboard  Esc back");return;
        }
        char row[39];
        if(page==Wifi){
            frame("WI-FI");char status[23];strlcpy(status,state.online?"CONNECTED":wifiMessage(weather.wifi.phase),sizeof(status));text(6,32,status,1,state.online?theme::Success:theme::Attention);
            text(6,44,state.ssid[0]?state.ssid:"No current SSID");snprintf(row,sizeof(row),"IP %s",state.online?state.ip:"--");text(6,56,row);
            if(state.online)snprintf(row,sizeof(row),"RSSI %d dBm",state.rssi);
            else strlcpy(row,"RSSI --",sizeof(row));
            text(144,32,row,1,theme::Attention);
            text(6,69,"SAVED NETWORKS",1,theme::Primary);
            if(!weather.knownNetworks)text(6,81,"None saved; A adds a network",1,theme::Attention);
            unsigned start=networkSelected>=3?networkSelected-2:0;
            for(unsigned i=start;i<weather.knownNetworks && i<start+3;++i){text(6,81+int(i-start)*10,i==networkSelected?">":" ",1,theme::Primary);text(18,81+int(i-start)*10,weather.knownSsids[i]);}
            text(6,112,weather.networkSaveFailed?"Network storage error":message[0]?message:wifiMessage(weather.wifi.phase),1,weather.networkSaveFailed?theme::Error:theme::Attention);
            text(6,126,"Enter join  A scan  D forget",1,theme::Primary);return;
        }
        if(page==Display){frame("DISPLAY");snprintf(row,sizeof(row),"Brightness: %d / 255",brightness);text(6,36,row,1,theme::Primary);text(6,50,"Arrows adjust; Enter saves");
            snprintf(row,sizeof(row),"H Home theme: %s",homeThemeName(homeTheme()));text(6,70,row,1,theme::Primary);
            text(6,84,twelveHourClock()?"T Clock: 12-hour (toggle/save)":"T Clock: 24-hour (toggle/save)",1,theme::Primary);
            text(6,104,message,1,!strcmp(message,"Settings save failed")?theme::Error:theme::Success);text(6,126,"Esc back");return;}
        if(page==HomeThemePage){
            frame("HOME THEME");
            for(unsigned i=0;i<HomeThemeCount;++i){
                const HomeTheme t=HomeTheme(i);const bool on=i==themeSelected,current=t==homeTheme();const int y=32+int(i)*24;
                if(on)screen().drawRoundRect(2,y-3,236,22,3,theme::Primary);
                snprintf(row,sizeof(row),"%u %s%s",i+1,homeThemeName(t),current?"  (current)":"");
                text(8,y,row,1,on?theme::Primary:theme::Content);text(20,y+10,homeThemeAbout(t),1,0x8410);
            }
            text(6,108,message,1,!strcmp(message,"Home theme saved")?theme::Success:theme::Error);
            text(6,126,"Up/Down  Enter apply  Esc back",1,theme::Primary);return;
        }
        if(page==Shortcuts){
            // Reference only: arrows, Enter, Esc and G0 (Home) cover normal use.
            frame("SHORTCUT LIST");
            text(6,30,"Fn+1 Cache      Fn+2 Weather",1,theme::Primary);
            text(6,43,"Fn+3 Crypto     Fn+4 Terminal",1,theme::Primary);
            text(6,56,"Fn+5 Settings   Fn+6 Calendar",1,theme::Primary);
            text(6,69,"Fn+S SSH",1,theme::Primary);
            text(6,83,"Fn+L   Idle screen");
            text(6,95,"Fn+Esc Pocket sleep (G0 wakes)");
            text(6,109,"Menus: arrows, Enter select, G0 Home",1,theme::RouteSettings);
            text(6,126,"Esc back");return;
        }
        if(page==About){frame("ABOUT");snprintf(row,sizeof(row),"%s %s",ProductName,SparkVersion);text(6,34,row,1,theme::Primary);text(6,51,"M5Stack Cardputer ADV / ESP32-S3");text(6,68,"Build " __DATE__);text(6,85,"SD local console; SSH reserved");text(6,102,"Productivity / everyday companion");text(6,126,"Esc back");return;}
        frame("MAINTENANCE");
        if(!maintenancePage){
            snprintf(row,sizeof(row),"%s %s",ProductName,SparkVersion);text(6,32,row,1,theme::Primary);
            snprintf(row,sizeof(row),"Uptime %lu h %lu m %lu s",static_cast<unsigned long>(state.uptime/3600),static_cast<unsigned long>((state.uptime/60)%60),static_cast<unsigned long>(state.uptime%60));text(6,45,row);
            snprintf(row,sizeof(row),"Heap %lu B  min %lu B",static_cast<unsigned long>(state.heap),static_cast<unsigned long>(state.minimumHeap));text(6,58,row);
            snprintf(row,sizeof(row),"Battery %d%%",state.battery);text(6,71,state.battery<0?"Battery --%":row,1,state.battery>=0&&state.battery<=15?theme::Error:theme::Content);
            text(6,84,state.timeReady?"Time synchronized / local reminders":"Time sync pending; reminders waiting",1,state.timeReady?theme::Success:theme::Attention);
            weatherFreshness(weather,time(nullptr),row,sizeof(row));text(6,97,row,1,theme::Attention);
        }else{
            text(6,32,state.online?"Wi-Fi connected":"Wi-Fi disconnected",1,state.online?theme::Success:theme::Error);
            text(6,45,state.ssid[0]?state.ssid:"No current SSID");
            if(state.online)snprintf(row,sizeof(row),"IP %s  RSSI %d dBm",state.ip,state.rssi);
            else snprintf(row,sizeof(row),"IP --  RSSI --");
            text(6,58,row);
            text(6,71,state.storage.mounted?"SD mounted":"SD unavailable",1,state.storage.mounted?theme::Success:theme::Error);
            snprintf(row,sizeof(row),"SD total %llu MB",static_cast<unsigned long long>(state.storage.total/1048576));text(6,84,state.storage.mounted&&state.storage.capacityValid?row:"SD total --");
            snprintf(row,sizeof(row),"SD free  %llu MB",static_cast<unsigned long long>((state.storage.total>=state.storage.used?state.storage.total-state.storage.used:0)/1048576));text(6,97,state.storage.mounted&&state.storage.capacityValid?row:"SD free --");
        }
        text(6,112,message,1,theme::Attention);text(6,126,"Left/right  R restart  W reconnect",1,theme::Primary);
    }
};
}
