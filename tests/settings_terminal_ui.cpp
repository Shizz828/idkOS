#include <cassert>
#include "../src/apps/SettingsApp.h"
#include "../src/apps/TerminalApp.h"
struct Network {
    unsigned calls=0;uint8_t action=0;unsigned selected=0;
    bool networkAction(uint8_t a,unsigned i=0){++calls;action=a;selected=i;return true;}
};
struct Storage {
    spark::TerminalResult result;unsigned calls=0;bool approved=false;char command[161]{};
    bool submitTerminal(const char* input,const char*,bool confirmation){++calls;approved=confirmation;strcpy(command,input);result.busy=true;return true;}
    bool submitSystemTerminal(const char* input,const char* cwd){return submitTerminal(input,cwd,false);}
    bool terminalSnapshot(spark::TerminalResult& out){out=result;return true;}
};
int main(){
    Network network;spark::WeatherSnapshot weather;weather.knownNetworks=3;
    strcpy(weather.knownSsids[0],"Home test network");strcpy(weather.knownSsids[1],"Hotspot test");strcpy(weather.knownSsids[2],"Office");
    spark::SystemSnapshot state;state.uptime=123456;state.heap=180000;state.minimumHeap=120000;state.battery=12;state.online=true;state.rssi=-100;
    snprintf(state.ssid,sizeof(state.ssid),"%.32s","A network name with 32 characters");strcpy(state.ip,"255.255.255.255");state.storage.mounted=true;state.storage.capacityValid=true;state.storage.total=uint64_t(128)*1024*1024*1024;
    spark::SettingsApp settings;spark::InputEvent e;
    for(unsigned page=0;page<=5;++page){settings.page=spark::SettingsApp::Page(page);settings.draw(weather,state);}
    // Visible branding is idkOS; nothing on these screens still says Spark.
    auto shows=[](const char* value){for(const auto& row:M5Cardputer.Display.printed)if(row.text==value)return true;return false;};
    auto spark=[]{for(const auto& row:M5Cardputer.Display.printed)if(row.text.find("Spark")!=std::string::npos || row.text.find("SPARK")!=std::string::npos)return true;return false;};
    assert(!strcmp(spark::ProductName,"idkOS"));
    settings.page=spark::SettingsApp::About;settings.draw(weather,state);assert(shows("idkOS 1.3.0") && !spark());
    settings.page=spark::SettingsApp::Maintenance;settings.draw(weather,state);assert(shows("idkOS 1.3.0") && !spark());
    settings.page=spark::SettingsApp::Main;settings.draw(weather,state);assert(shows("4 Shortcut List") && shows("5 About idkOS") && !spark());
    // Settings > 4 Shortcut List and 5 About idkOS (always last): by number, or by arrows and Enter; Esc returns.
    settings.page=spark::SettingsApp::Main;strcpy(e.text,"5");settings.input(e,weather,network);
    assert(settings.page==spark::SettingsApp::About);assert(settings.back() && settings.page==spark::SettingsApp::Main);
    strcpy(e.text,"4");settings.input(e,weather,network);
    assert(settings.page==spark::SettingsApp::Shortcuts);assert(settings.back() && settings.page==spark::SettingsApp::Main);
    e={};e.down=true;settings.input(e,weather,network); // 4 -> 5...
    e={};e.down=true;settings.input(e,weather,network); // ...5 wraps to 1...
    e={};e.up=true;settings.input(e,weather,network);   // ...and back to 5, the last row.
    e={};e.enter=true;settings.input(e,weather,network);assert(settings.page==spark::SettingsApp::About);
    settings.draw(weather,state);settings.back();
    e={};e.up=true;settings.input(e,weather,network);e={};e.enter=true;settings.input(e,weather,network);
    assert(settings.page==spark::SettingsApp::Shortcuts);settings.back();e={};
    settings.page=spark::SettingsApp::Maintenance;e.right=true;settings.input(e,weather,network);settings.draw(weather,state);
    e={};strcpy(e.text,"r");settings.input(e,weather,network);settings.draw(weather,state);e={};e.enter=true;assert(settings.input(e,weather,network)==spark::SettingsAction::None);
    e={};strcpy(e.text,"r");settings.input(e,weather,network);e.text[0]='y';assert(settings.input(e,weather,network)==spark::SettingsAction::Restart);
    e={};strcpy(e.text,"r");settings.input(e,weather,network);settings.cancelRestart();
    e.text[0]='y';assert(settings.input(e,weather,network)==spark::SettingsAction::None);
    settings.openWifi();assert(settings.page==spark::SettingsApp::Wifi);
    e={};e.text[0]='a';assert(settings.input(e,weather,network)==spark::SettingsAction::AddWifi);
    e.text[0]='s';assert(settings.input(e,weather,network)==spark::SettingsAction::AddWifi);
    state.online=false;weather.wifi.start(1,0);settings.draw(weather,state);
    weather.wifi.update(false,202,1);settings.draw(weather,state);
    state.online=true;
    e={};strcpy(e.text,"d");settings.input(e,weather,network);settings.draw(weather,state);e={};e.enter=true;settings.input(e,weather,network);assert(network.calls==0);
    e={};e.text[0]='d';settings.input(e,weather,network);e.text[0]='y';settings.input(e,weather,network);assert(network.calls==1&&network.action==2);
    e={};e.enter=true;settings.input(e,weather,network);assert(network.action==1);
    settings.page=spark::SettingsApp::Display;for(int i=0;i<100;++i){e={};e.left=true;settings.input(e,weather,network);}assert(settings.brightness==20);
    for(int i=0;i<100;++i){e={};e.right=true;settings.input(e,weather,network);}assert(settings.brightness==255);
    e={};e.text[0]='t';assert(settings.input(e,weather,network)==spark::SettingsAction::SaveClockFormat);
    e.text[0]='T';assert(settings.input(e,weather,network)==spark::SettingsAction::SaveClockFormat);
    e={};e.enter=true;assert(settings.input(e,weather,network)==spark::SettingsAction::SaveBrightness);
    for(bool twelve:{false,true}){spark::twelveHourClock()=twelve;settings.savedClockFormat(true);settings.draw(weather,state);settings.savedClockFormat(false);settings.draw(weather,state);}
    spark::twelveHourClock()=false;
    weather.knownNetworks=5;strcpy(weather.knownSsids[3],"Fourth");strcpy(weather.knownSsids[4],"Fifth");
    settings.page=spark::SettingsApp::Wifi;
    for(int i=0;i<4;++i){e={};e.down=true;settings.input(e,weather,network);}
    settings.draw(weather,state);e={};e.enter=true;settings.input(e,weather,network);assert(network.selected==4);
    Storage storage;spark::TerminalApp terminal;terminal.draw();e={};strcpy(e.text,"rm test.txt");e.enter=true;terminal.input(e,storage);terminal.draw();assert(storage.calls==0);
    e={};e.enter=true;terminal.input(e,storage);assert(storage.calls==0);
    e={};strcpy(e.text,"rm test.txt");e.enter=true;terminal.input(e,storage);e={};e.text[0]='y';terminal.input(e,storage);assert(storage.calls==1&&storage.approved);
    storage.result.busy=false;storage.result.revision=2;storage.result.line("Deleted");assert(terminal.poll(storage,100));terminal.draw();
    e={};strcpy(e.text,"cd ../../");e.enter=true;terminal.input(e,storage);assert(storage.calls==2);
    storage.result.busy=false;storage.result.revision=3;terminal.poll(storage,200);
    e={};strcpy(e.text,"rm -r folder");e.enter=true;terminal.input(e,storage);assert(storage.calls==2);
    e={};e.enter=true;terminal.input(e,storage);assert(storage.calls==2);
    e={};strcpy(e.text,"rmdir folder");e.enter=true;terminal.input(e,storage);assert(storage.calls==2);
    e={};strcpy(e.text,"y");terminal.input(e,storage);assert(storage.calls==3 && storage.approved);
    storage.result.busy=false;storage.result.revision=4;terminal.poll(storage,300);
    assert(terminal.setDeveloper(true));e={};strcpy(e.text,"rm /info");e.enter=true;terminal.input(e,storage);assert(storage.calls==4 && !storage.approved);
    settings.page=spark::SettingsApp::Main;e={};strcpy(e.text,"d");assert(settings.input(e,weather,network)==spark::SettingsAction::Developer);

}
