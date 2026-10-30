#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "apps/TerminalApp.h"
#include "apps/SshApp.h"
#include "services/SshService.h"
#include "apps/SettingsApp.h"
#include "core/Input.h"
#include "core/ScreenSleep.h"
#include "core/IdleBacklight.h"
#include "core/LightSleep.h"
#include "core/IdleBoard.h"
#include "core/MotionSensor.h"
#include "ui/DisplayRecovery.h"
#include <esp_system.h>
#include <esp_sleep.h>
#include "core/AppRegistry.h"
#include "ui/Screen.h"
#include "apps/WeatherHub.h"
#include "apps/WifiSetup.h"
#include "services/WeatherService.h"
#include "services/CryptoService.h"
#include "apps/CryptoApp.h"
#include "core/CryptoProvisioning.h"
#include "apps/CaptureApp.h"
#include "storage/NoteStorage.h"
#include "ui/StatusBar.h"
#include "ui/IdleBoardScreen.h"
#include "services/ReminderService.h"
#include "ui/ReminderNotice.h"
#include "core/SystemTime.h"
#include "core/BatteryEstimate.h"
#include "core/PowerPolicy.h"
#include "core/UiHealth.h"
#include "core/ReminderTone.h"
#include "core/ReminderWake.h"
#include "core/StartupSequence.h"
#include "ui/StartupScreen.h"
#include "apps/LocationSettings.h"
using namespace spark;
namespace {
Navigation nav;
Input input;
WeatherSnapshot weatherCache;
WeatherService weatherService;
WifiSetup wifiSetup;
CryptoSnapshot cryptoCache;
CryptoService cryptoService;
CryptoNavigation cryptoNavigation;
CryptoProvisioning cryptoProvisioning;
NoteStorage noteStorage;
CaptureApp<NoteStorage> capture;
uint32_t lastCapturePoll=0;
uint32_t lastWeatherPoll=0, lastWeatherDraw=0;
static_assert(sizeof(Apps)/sizeof(Apps[0])==AppCount,"App registry count mismatch");
bool dirty=true, diagnostics=false;
uint32_t lastCaptureLog=0;
char typed[65]{};
const char* lastKey="Ready";
bool shiftHeld=false, fnHeld=false;
uint32_t lastStatus=0,lastStatusPoll=0;
StatusState statusState;
BatteryEstimate batteryEstimate;
PowerAudit powerAudit;
uint32_t lastBatteryPoll=0,lastPowerLog=0;
int rawBattery=-1,batteryMv=-1,batteryLevel=-1;
ReminderTone reminderTone;
ReminderWake reminderWake;
ScreenSleep screenSleep;
IdleBacklight idleBacklight;
IdleBoard idleBoard;
MotionSensor motionSensor;
bool screenOff=false;
uint64_t standbyWakeKeys=0;
uint32_t nextStandbyAttempt=0;
bool standbyLogged=false;
ReminderService reminders;
StartupSequence startup;
LocationSettings locationSettings;
TerminalApp terminal;
SshApp sshApp;
SshService sshService;
uint32_t lastSshPoll=0;
SettingsApp settings;
SystemSnapshot systemState;
uint32_t lastSystemPoll=0,lastStatsRequest=0;
bool reminderWasVisible=false;
uint32_t lastReminderPoll=0;
void applyBacklight(uint32_t now) {
    uint8_t brightness=idleBoard.active()?idleBoard.brightness(now,settings.brightness,screenOff)
                                         :idleBacklight.brightness(now,settings.brightness,screenOff);
    if(M5Cardputer.Display.getBrightness()!=brightness)M5Cardputer.Display.setBrightness(brightness);
}
void wakeScreen(uint32_t now,const char* source) {
    idleBacklight.activity(now);
    weatherService.setStandby(false);
    uiHealth().mark(UiPhase::Display,millis());
    bool ok=recoverDisplay(M5Cardputer.Display,settings.brightness);
    uiHealth().mark(UiPhase::Services,millis());
    Serial.printf("[screen] wake source=%s display=%d\n",source,ok);
    dirty=true;
}
void drawHome(time_t now) {
    frame(ProductName);
    char glance[40];
    if(now>=ClockMinimum) {
        struct tm local{},utc{}; localtime_r(&now,&local); gmtime_r(&now,&utc);
        formatClock(glance,sizeof(glance),local.tm_hour,local.tm_min); text(6,31,glance,2);
        char utcClock[9];formatClock(utcClock,sizeof(utcClock),utc.tm_hour,utc.tm_min);
        snprintf(glance,sizeof(glance),"UTC %s",utcClock); text(150,34,glance);
        strftime(glance,sizeof(glance),"%a %b %d %Y",&local); text(6,52,glance);
    } else { text(6,31,"--:--",2); text(150,34,"UTC --:--"); text(6,52,"Date: not set"); }
    if(weatherCache.weather.valid) {
        snprintf(glance,sizeof(glance),"%dF %s%s",int(lroundf(weatherCache.weather.temperature)),
            condition(weatherCache.weather.code),(!weatherCache.online || weatherCache.weatherFailed || stale(weatherCache.weather.updatedAt,now))?" [cached]":"");
        glance[37]=0; text(6,66,glance);
    } else text(6,66,"Weather: unavailable");
    if(weatherCache.weather.valid) {
        char location[18]{};
        memcpy(location,weatherCache.location.label,strnlen(weatherCache.location.label,sizeof(location)-1));
        text(132,52,location,1,theme::Primary);
    }
    const uint16_t routeColors[]={theme::RouteCapture,theme::RouteWeather,theme::RouteCrypto,
        theme::RouteTerminal,theme::RouteSettings};
    const char routeLetters[]={'C','W','C','T','S'};
    // Five icons, 46 px apart and centered on the 240 px screen (centers 28..212),
    // so the highlight ring keeps 14 px from both edges and clears the weather line.
    constexpr int IconGap=46,IconRowY=94;
    for(unsigned i=0;i<5;++i) {
        const int x=120+IconGap*(int(i)-2);
        auto& d=screen();
        d.fillCircle(x,IconRowY,10,routeColors[i]);
        char badge[2]={routeLetters[i],0};
        d.setTextSize(2);
        d.setTextColor(i<2?theme::Content:theme::Background,routeColors[i]);
        d.setCursor(x-6,IconRowY-8);
        d.print(badge);
        if(i==nav.selected){d.drawCircle(x,IconRowY,13,theme::Primary);d.drawCircle(x,IconRowY,14,theme::Primary);}
    }
    // Left/Right highlight an app and G0 or Enter opens it; Fn shortcuts are in Settings.
    if(nav.selected<HomeAppCount) {
        const char* label=Apps[nav.selected].label;
        text(120-int(strlen(label))*3,116,label,1,routeColors[nav.selected]);
    }
}
// Opens an app from the Home launcher or a Fn shortcut; false when the terminal cannot start.
bool openApp(unsigned target) {
    if(target==ConsoleIndex && !terminal.setDeveloper(false))return false;
    nav.selected=target;nav.open();if(target==CaptureIndex)capture.open(noteStorage);dirty=true;
    return true;
}
void drawApp(int64_t now) {
    if(nav.active==SshIndex) {sshApp.draw();return;}
    if(nav.active==ConsoleIndex) {terminal.draw();return;}
    if(nav.active==SettingsIndex && !diagnostics && !locationSettings.active) {
        if(wifiSetup.editing) wifiSetup.draw(weatherCache,"ADD WI-FI");
        else settings.draw(weatherCache,systemState);
        return;
    }
    if(nav.active==SettingsIndex && locationSettings.active) { locationSettings.draw(weatherCache); return; }
    if(nav.active==CaptureIndex) { capture.draw(); return; }
    if(nav.active==CryptoIndex) { drawCrypto(cryptoCache,cryptoNavigation,now); return; }
    if(nav.active==WeatherIndex) { drawWeatherHub(weatherCache,now); return; }
    if (diagnostics) {
        frame("KEYBOARD TEST");
        char modifiers[40];
        snprintf(modifiers,sizeof(modifiers),"Shift: %s   Fn: %s",
            shiftHeld ? "ON" : "off", fnHeld ? "ON" : "off");
        text(6,32,modifiers);
        text(6,46,"Hold Shift + letter: capitals");
        text(6,62,"Last:"); text(48,62,lastKey);
        // Fixed 32-column viewport; full buffer remains in RAM.
        char line[33]{}; size_t len=strlen(typed);
        memcpy(line,typed+(len>32 ? len-32 : 0),len>32 ? 32 : len);
        text(6,83,line);
        text(6,106,"Backspace edits; Enter clears");
        text(6,123,"G0 home    RAM only");
        return;
    }
    const auto& app=Apps[nav.active];
    frame(app.label);
    text(6,38,nav.active==ConsoleIndex ? "ESP32 LOCAL CONSOLE" : "App planned for V1");
    text(6,60,"Not implemented in this build");
    text(6,84,"Keyboard + launcher prototype");
    text(6,123,"Esc back     G0 home");
}
}
void setup() {
    uint32_t bootStarted=millis();
    auto cfg=M5.config();
    M5Cardputer.begin(cfg,true);
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setTextWrap(false);
    Preferences displayPrefs;
    if(displayPrefs.begin("spark-display",true)){settings.brightness=displayPrefs.getUChar("brightness",100);loadClockFormat(displayPrefs);displayPrefs.end();}
    if(settings.brightness<20)settings.brightness=20;
    M5Cardputer.Display.setBrightness(settings.brightness);
    // Reinitialize the existing panel on every boot, including software resets
    // while its previous backlight was off. No USB/power edge is needed.
    bool displayReady=recoverDisplay(M5Cardputer.Display,settings.brightness);
    Serial.begin(115200);
    Serial.printf("[boot] reset=%d wake=%d display=%d\n",int(esp_reset_reason()),int(esp_sleep_get_wakeup_cause()),displayReady);
    Serial.printf("%s %s board=%d ADV=%d heap=%u\n",ProductName,
        SparkVersion,
        int(M5.getBoard()),int(m5::board_t::board_M5CardputerADV),ESP.getFreeHeap());
    if (M5.getBoard()!=m5::board_t::board_M5CardputerADV) {
        frame("DEVICE CHECK");
        text(6,40,"ADV not detected");
        text(6,62,"Check hardware/library setup");
        while (true) { M5Cardputer.update(); delay(5); }
    }
    Serial.printf("[motion] BMI270=%u sample_ms=%lu; Fn+Esc motion off\n",motionSensor.begin(),(unsigned long)MotionSampleMs);
    if(!initScreenBuffer()) Serial.println("Display buffer unavailable; direct rendering");
    renderScreen([]{drawStartupScreen();});
    presentStartupLogo();
    startup.begin(millis(),M5Cardputer.Speaker);
    Serial.printf("[startup] speaker_started=%u target=%u warmup_ms=%lu\n",startup.soundActive(),unsigned(StartupChimeVolume),(unsigned long)StartupAudioWarmupMs);
    reminders.begin();
    if(!sshService.begin())Serial.println("[ssh] worker initialization failed");
    if(noteStorage.begin()) noteStorage.submit(NoteOperation::List);
    bool cryptoReady=cryptoService.begin();
    if(!cryptoReady) snprintf(cryptoCache.status,sizeof(cryptoCache.status),"Crypto service memory low");
    if(!weatherService.begin(cryptoReady?&cryptoService:nullptr)) {
        snprintf(weatherCache.status,sizeof(weatherCache.status),"Weather service memory low");
        snprintf(cryptoCache.status,sizeof(cryptoCache.status),"Network service unavailable");
    }
    Serial.printf("[boot] setup_ms=%lu\n",(unsigned long)(millis()-bootStarted));
}
void loop() {
    uiHealth().mark(UiPhase::Input,millis());
    M5Cardputer.update();
    uiHealth().mark(UiPhase::Services,millis());
    cryptoProvisioning.poll(cryptoService);
    uint32_t now=millis();
    // Plain arrows navigate Home, menus and lists. Terminal, SSH, text entry and
    // the keyboard test keep their existing keys.
    bool menuMode=nav.active!=SshIndex && nav.active!=ConsoleIndex && !diagnostics &&
        !(nav.active==CaptureIndex && capture.textFocused() && !capture.reminderForm()) &&
        !(nav.active==SettingsIndex && (locationSettings.active || (wifiSetup.editing && !wifiSetup.choosingNetwork())));
    // Every non-menu screen is text entry (Terminal, SSH included): Shift+Esc types '~'.
    auto e=input.poll(now,!menuMode,menuMode);
    if(nav.active==SshIndex && e.enter)Serial.printf("[ssh-router] Enter detected before global routing sleep=%u quarantine=%u\n",screenOff,screenSleep.quarantined());
    e.home=e.home || (standbyWakeKeys&StandbyHomeMask);
    e.keyPressed=e.keyPressed || (standbyWakeKeys&StandbyKeyboardMask);
    standbyWakeKeys=0;
    // Service tasks initialize and synchronize time behind the splash. Ignore
    // startup keypresses so it always transitions to the existing Home screen.
    if(startup.showing()) {
        if(!startup.update(now,M5Cardputer.Speaker)){delay(2);return;}
        Serial.printf("[startup] complete audio=%s\n",startup.audioStatus());
        idleBacklight.activity(now);
        dirty=true;e={};
    }
    // Observe physical input before manual-sleep quarantine or app/reminder
    // handling consumes it. Held keys and releases also reset the idle timers.
    bool physical=e.changed || e.keyPressed || e.home || !M5Cardputer.Keyboard.keyList().empty() || M5Cardputer.BtnA.isPressed();
    bool autoWasOff=idleBacklight.screenOff();
    if(physical)idleBacklight.activity(now);
    // Input relights a dark idle board or automatic screen-off; that wake gesture is quarantined below.
    bool wasPhysicalOff=screenOff,boardWasDark=idleBoard.motionReady();
    if(physical)idleBoard.show(now);
    reminderTone.update(now,M5Cardputer.Speaker);
    // The accelerometer samples only while the idle board or automatic screen-off
    // is dark, never in Fn+Esc sleep.
    bool motionWake=motionSensor.poll(now,!screenSleep.sleeping() && (boardWasDark || autoWasOff));
    if(motionWake){idleBoard.show(now);idleBacklight.activity(now);Serial.println("[motion] pickup confirmed; screen on");}
    bool wake=false;
    bool homeWake=e.home;
    bool wasOff=screenSleep.sleeping();
    bool homeRecovery=e.home && !wasOff && screenSleep.quarantined();
    // IRQ stays low until queued keyboard events have drained. Include it in
    // release detection so a quick tap cannot leak from the wake FIFO into apps.
    bool anyHeld=!M5Cardputer.Keyboard.keyList().empty() || M5Cardputer.BtnA.isPressed() || standbyInputPending();
    bool homeHeld=M5Cardputer.BtnA.isPressed() || !gpio_get_level(StandbyHomePin);
    if(screenSleep.filter(e.sleep,e.keyPressed,e.home,anyHeld,wake,homeHeld))e={};
    else if(homeRecovery){e={};e.home=true;Serial.println("[input] G0 released wake quarantine; draft retained");}
    if(!wasOff && reminderWake.expired(now) && screenSleep.reminderSleep())
        Serial.println("[reminder] brief wake ended; screen off");
    bool manualOff=screenSleep.sleeping();
    // Five minutes without input on any screen: screen off over the current app, no lock.
    if(!idleBacklight.screenOff() && idleBacklight.update(now,manualOff || idleBoard.active())) {
        settings.cancelRestart();
        Serial.println("[idle] 5 minutes inactive; screen off");
    }
    idleBoard.update(now,manualOff,motionSensor.enabled());
    if(!idleBoard.motionReady() && !idleBacklight.screenOff())motionSensor.poll(now,false);
    screenOff=manualOff || idleBoard.motionReady() || idleBacklight.screenOff();
    if(!wasOff && manualOff) {
        settings.cancelRestart();
        M5Cardputer.Display.setBrightness(0);
        M5Cardputer.Display.sleep();
        weatherService.setStandby(true);sshService.disconnect();
        nextStandbyAttempt=now;standbyLogged=false;
        Serial.println("[screen] sleep; preparing low-power standby");
    } else if(!wasPhysicalOff && screenOff) {
        // Dark idle board or automatic screen-off: panel off; the network pauses unless SSH is active.
        M5Cardputer.Display.setBrightness(0);
        M5Cardputer.Display.sleep();
        if(!sshService.busy())weatherService.setStandby(true);
        nextStandbyAttempt=now;standbyLogged=false;
        Serial.println("[idle] screen dark; motion-ready, BMI270 timer polling");
    }
    if((boardWasDark || autoWasOff) && !screenOff){screenSleep.quarantineWake();e={};wake=true;}
    if(wake) wakeScreen(now,motionWake?"motion":homeWake?"G0":"keyboard");
    if(screenOff && !idleBacklight.screenOff())idleBacklight.activity(now);
    applyBacklight(now);
    // Fn+L shows the idle board over the retained screen. Esc/Enter return to it; G0 goes Home.
    bool idleShown=idleBoard.active();
    if(e.idle && !screenOff && !idleBoard.active()) {
        idleBoard.enter(now);settings.cancelRestart();dirty=true;e={};
        Serial.println("[idle] Fn+L; idle board");
    } else if(idleBoard.active() && !screenOff && !reminderWasVisible) {
        bool leave=e.back || e.enter || e.home,home=e.home;
        if(leave){idleBoard.leave();idleBacklight.activity(now);dirty=true;Serial.println("[idle] board closed");}
        e={};e.home=leave && home;
    }
    bool statusChanged=false;
    if(!powerAudit.batteryReads || uint32_t(now-lastBatteryPoll)>=BatteryPollMs) {
        lastBatteryPoll=now;++powerAudit.batteryReads;
        rawBattery=M5Cardputer.Power.getBatteryLevel();batteryMv=M5Cardputer.Power.getBatteryVoltage();
        batteryLevel=batteryEstimate.update(rawBattery,now);
    }
    if(uint32_t(now-lastStatusPoll)>=1000) {
        lastStatusPoll=now; StatusState fresh; fresh.online=WiFi.status()==WL_CONNECTED; fresh.battery=batteryLevel;
        int64_t statusNow=time(nullptr);fresh.clockMinute=statusClockMinute(statusNow,systemTimeReady(statusNow));
        systemState.battery=fresh.battery;
        statusChanged=fresh.online!=statusState.online || fresh.battery!=statusState.battery || fresh.clockMinute!=statusState.clockMinute; statusState=fresh;
    }
    if(statusChanged && (idleBoard.active() || nav.active==SettingsIndex || nav.active<0))dirty=true;
    shiftHeld=e.shift; fnHeld=e.fn;
    if(uint32_t(now-lastWeatherPoll)>=250) {
        lastWeatherPoll=now;
        WeatherSnapshot next;
        if(weatherService.snapshot(next)) {
            if(next.revision!=weatherCache.revision && (idleBoard.active() || nav.active==WeatherIndex || nav.active==SettingsIndex || nav.active<0)) dirty=true;
            weatherCache=next;
            wifiSetup.sync(next);
        }
        CryptoSnapshot market;
        if(cryptoService.snapshot(market)) {
            if(market.revision!=cryptoCache.revision && nav.active==CryptoIndex) dirty=true;
            cryptoCache=market;
        }
    }
    if(uint32_t(now-lastCapturePoll)>=100) {
        lastCapturePoll=now;
        if(capture.poll(noteStorage,now,nav.active==CaptureIndex && !screenOff)) { reminders.sync(capture.storageState()); if(nav.active==CaptureIndex) dirty=true; }
    }
    if(terminal.poll(noteStorage,now) && nav.active==ConsoleIndex)dirty=true;
    if(uint32_t(now-lastSshPoll)>=50){lastSshPoll=now;if(sshApp.poll(sshService) && nav.active==SshIndex)dirty=true;}
    if(!screenOff && nav.active==SettingsIndex && (settings.page==SettingsApp::Maintenance || settings.page==SettingsApp::Wifi) && uint32_t(now-lastSystemPoll)>=1000) {
        lastSystemPoll=now;systemState.uptime=now/1000;systemState.heap=ESP.getFreeHeap();systemState.minimumHeap=ESP.getMinFreeHeap();
        systemState.battery=statusState.battery;systemState.online=WiFi.status()==WL_CONNECTED;systemState.timeReady=systemTimeReady(time(nullptr));
        strlcpy(systemState.ssid,WiFi.SSID().c_str(),sizeof(systemState.ssid));strlcpy(systemState.ip,WiFi.localIP().toString().c_str(),sizeof(systemState.ip));
        systemState.rssi=systemState.online?WiFi.RSSI():0;noteStorage.storageSnapshot(systemState.storage);
        if(nav.active==SettingsIndex)dirty=true;
    }
    if(!screenOff && nav.active==SettingsIndex && settings.page==SettingsApp::Maintenance && uint32_t(now-lastStatsRequest)>=30000 && noteStorage.requestStats())lastStatsRequest=now;
    int64_t clock=time(nullptr);
    if(uint32_t(now-lastReminderPoll)>=1000) { lastReminderPoll=now; if(reminders.poll(clock,systemTimeReady(clock))) dirty=true; if(systemTimeReady(clock))reminders.flush(noteStorage,now); }
    bool notice=reminders.engine.visible(clock,systemTimeReady(clock));
    bool relit=notice && !reminderWasVisible && (idleBoard.remind(now) || idleBacklight.wake(now));
    if(notice && !reminderWasVisible)reminderTone.play(now,M5Cardputer.Speaker);
    // A reminder due during screen sleep lights the screen; no reboot/startup.
    if(notice && !reminderWasVisible && screenOff && screenSleep.reminderWake()) {
        screenOff=false;reminderWake.woke(now);wakeScreen(now,"reminder");
    }
    // A reminder due on the dark idle board or automatic screen-off wakes the panel.
    if(relit){screenOff=false;wakeScreen(now,"reminder");}
    if(notice!=reminderWasVisible) { reminderWasVisible=notice; dirty=true; }
    if(notice) {
        if(nav.active==SshIndex && e.enter)Serial.println("[ssh-router] Enter consumed by visible reminder");
        ReminderKey key=reminderKey(e);
        if(key!=ReminderKey::None)reminderWake.reminderAction(now);
        if(key==ReminderKey::Dismiss) { reminders.engine.dismiss(); reminders.save(); dirty=true; e={}; }
        else if(key!=ReminderKey::None) {
            if(reminders.engine.snooze(clock,key==ReminderKey::Snooze10?10:30)) { reminders.save(); dirty=true; } e={};
        } else if(!e.home && !e.shortcut && !e.back) e={};
    }
    // Any other key returns a brief reminder wake to normal use.
    if(e.keyPressed || e.home)reminderWake.userActivity();
    // Under the idle board, keys reach only a visible reminder, never the app.
    if(idleBoard.active())e={};
    if(nav.active==CaptureIndex && (e.enter || e.save || e.reminder || (e.erase && uint32_t(now-lastCaptureLog)>1000))) {
        lastCaptureLog=now; Serial.printf("[capture] enter=%u erase=%u save=%u focus=%u heap=%u\n",e.enter,e.erase,e.save,capture.textFocused(),ESP.getFreeHeap());
    }
    if(nav.active==SshIndex && e.back) {
        if(!sshApp.back(sshService))nav.home();
        dirty=true;
    } else if(nav.active==CaptureIndex && (e.home || e.back)) {
        if(capture.leave(e.home,noteStorage)) nav.home();
        dirty=true;
    } else if (e.home || e.back) {
        if(e.home && nav.active==ConsoleIndex)terminal.cancel();
        if(e.home && nav.active==SshIndex)sshApp.leave();
        if(e.back && locationSettings.active) locationSettings.cancel();
        else if(e.back && wifiSetup.editing) wifiSetup.cancel();
        else if(e.back && nav.active==ConsoleIndex && terminal.cancel()) {}
        else if (e.back && diagnostics) diagnostics=false;
        else if(e.back && nav.active==SettingsIndex && settings.back()) {}
        else { diagnostics=false; nav.home(); }
        if(e.home) { wifiSetup.cancel(); locationSettings.cancel(); }
        dirty=true;
    } else if(e.shortcut=='s') {
        // Fn+S preserves the current app's in-memory draft/form, like Home.
        if(nav.active!=SshIndex){diagnostics=false;nav.selected=SshIndex;nav.open();sshApp.open(sshService);dirty=true;}
    } else if(e.shortcut && !diagnostics && !locationSettings.active && !wifiSetup.editing && !(nav.active==CaptureIndex && capture.textFocused()) && nav.active!=SshIndex) {
        int target=shortcutApp(e.shortcut);
        if(target>=0 && !openApp(unsigned(target)))return;
    } else if (nav.active<0) {
        if(e.left || e.right){nav.moveHome(e.right?1:-1);dirty=true;}
        if(e.enter && !openApp(nav.selected<HomeAppCount?nav.selected:CaptureIndex))return;
    } else if(nav.active==CaptureIndex) {
        capture.input(e,noteStorage);
        if(e.changed || e.erase || e.save || e.up || e.down || e.left || e.right || e.text[0] || e.enter) dirty=true;
    } else if(nav.active==SshIndex) {
        sshApp.input(e,sshService,WiFi.status()==WL_CONNECTED);
        if(e.changed||e.enter||e.erase||e.text[0]||e.control||e.tab||e.up||e.down||e.left||e.right)dirty=true;
    } else if(nav.active==ConsoleIndex) {
        terminal.input(e,noteStorage);
        if(e.changed||e.enter||e.erase||e.text[0]||e.up||e.down||e.left||e.right)dirty=true;
    } else if(nav.active==CryptoIndex) {
        if(e.up) { cryptoNavigation.move(-1); dirty=true; }
        if(e.down) { cryptoNavigation.move(1); dirty=true; }
        if(e.left) { cryptoNavigation.page(-1); dirty=true; }
        if(e.right) { cryptoNavigation.page(1); dirty=true; }
    } else if(nav.active==WeatherIndex) {
        if(e.enter) {
            wifiSetup.cancel();locationSettings.cancel();diagnostics=false;
            settings.openWifi();nav.selected=SettingsIndex;nav.open();dirty=true;
        }
    } else if(nav.active==SettingsIndex && locationSettings.active) {
        locationSettings.input(e,weatherService); if(e.changed || e.text[0] || e.enter || e.save || e.erase || e.up || e.down) dirty=true;
    } else if (diagnostics) {
        if (e.up) lastKey="UP";
        if (e.down) lastKey="DOWN";
        if (e.left) lastKey="LEFT";
        if (e.right) lastKey="RIGHT";
        if (e.enter) { typed[0]=0; lastKey="ENTER"; }
        size_t len=strlen(typed);
        if (e.erase) {
            if (len) typed[--len]=0;
            lastKey="BACKSPACE";
        }
        for (const char* c=e.text; *c && len<sizeof(typed)-1; ++c) {
            typed[len++]=*c; typed[len]=0; lastKey="TEXT";
        }
        if (e.changed||e.up||e.down||e.left||e.right||e.enter||e.erase||e.text[0]) dirty=true;
    } else if (nav.active==SettingsIndex) {
        if(wifiSetup.editing) wifiSetup.input(e,weatherService);
        else {
            SettingsAction action=settings.input(e,weatherCache,weatherService);
            if(action==SettingsAction::AddWifi)wifiSetup.open(weatherService,weatherCache);
            if(action==SettingsAction::Location)locationSettings.open();
            if(action==SettingsAction::Keyboard)diagnostics=true;
            if(action==SettingsAction::Developer && terminal.setDeveloper(true)){nav.selected=ConsoleIndex;nav.open();}
            if(!screenOff && settings.page==SettingsApp::Display)applyBacklight(now);
            if(action==SettingsAction::SaveBrightness){Preferences prefs;bool ok=false;if(prefs.begin("spark-display",false)){ok=prefs.putUChar("brightness",settings.brightness)==1;prefs.end();}settings.savedBrightness(ok);}
            if(action==SettingsAction::SaveClockFormat){Preferences prefs;bool ok=false;if(prefs.begin("spark-display",false)){ok=saveClockFormat(prefs,!twelveHourClock());prefs.end();}settings.savedClockFormat(ok);}
            if(action==SettingsAction::Restart){if(noteStorage.idle()){Serial.println("[system] user-confirmed software restart");Serial.flush();ESP.restart();}else settings.storageBusy();}
        }
        if(e.changed||e.text[0]||e.enter||e.erase||e.up||e.down||e.left||e.right)dirty=true;
    }

    if (nav.active<0 && uint32_t(now-lastStatus)>=60000) { lastStatus=now; dirty=true; }
    if(nav.active==WeatherIndex && uint32_t(now-lastWeatherDraw)>=60000) { lastWeatherDraw=now; dirty=true; }
    if(nav.active==CryptoIndex && uint32_t(now-lastWeatherDraw)>=60000) { lastWeatherDraw=now; dirty=true; }
    bool fullDraw=dirty && !screenOff;
    uiHealth().mark(UiPhase::Display,millis());
    if (fullDraw) {
        renderScreen([&]{bool visible=reminders.engine.visible(clock,systemTimeReady(clock));
            if(idleBoard.active() && !visible){drawIdleBoard(statusState,weatherCache,clock,systemTimeReady(clock));return;}
            if(nav.active<0)drawHome(clock);else drawApp(clock);if(visible)drawReminderNotice(reminders.engine,reminders.saveFailed);drawStatus(statusState,nav.active>=0);});
        dirty=false;++powerAudit.redraws;
    }
    if(statusChanged && !screenOff && !fullDraw && !idleBoard.active())renderScreenRegion(142,0,98,12,[&]{drawStatus(statusState,nav.active>=0);});
    uiHealth().mark(UiPhase::Audio,millis());
    if(!reminderTone.active() && !M5Cardputer.Speaker.isPlaying() && M5Cardputer.Speaker.isRunning())M5Cardputer.Speaker.end();
    if(screenOff && IdleBoard::cpuSleepAllowed(screenSleep.sleeping(),bool(Serial)) && int32_t(now-nextStandbyAttempt)>=0 &&
       canEnterStandby(screenSleep.readyForStandby() || idleBoard.motionReady() || idleBacklight.screenOff(),anyHeld || standbyInputPending(),
                       weatherService.standbyReady(),noteStorage.idle(),sshService.busy(),
                       reminderTone.active() || M5Cardputer.Speaker.isPlaying())) {
        // Finish asynchronous speaker shutdown before stopping CPU/peripheral clocks.
        if(M5Cardputer.Speaker.isRunning())M5Cardputer.Speaker.end();
        if(!standbyLogged){Serial.println(screenSleep.sleeping()?"[power] light sleep; timer=1s keyboard=G11 G0=G0"
                                                               :"[power] light sleep; motion timer=250ms keyboard=G11 G0=G0");standbyLogged=true;}
        uint32_t sleepStarted=micros();++powerAudit.attempts;
        uiHealth().mark(UiPhase::Sleep,millis());
        auto result=enterLightSleep(screenSleep.sleeping()?StandbyIntervalUs:MotionSleepUs);
        if(result.entered && result.error==ESP_OK){++powerAudit.sleeps;powerAudit.sleepUs+=uint32_t(micros()-sleepStarted);}
        standbyWakeKeys=result.keys;
        if(result.error!=ESP_OK) {
            ++powerAudit.failures;
            Serial.printf("[power] light sleep failed=%d; retry in 5s\n",int(result.error));
            nextStandbyAttempt=millis()+5000;
        }
    }
    if(uint32_t(now-lastPowerLog)>=60000 || wake) {
        lastPowerLog=now;
        Serial.printf("[power] up_s=%lu off=%u mv=%d raw=%d shown=%d cpu=%u sleep_ms=%llu entries=%lu attempts=%lu errors=%lu draws=%lu battery_reads=%lu net_paused=%u input_blocked=%u irq=%u keys=%u\n",
            (unsigned long)(now/1000),screenOff,batteryMv,rawBattery,statusState.battery,getCpuFrequencyMhz(),
            (unsigned long long)(powerAudit.sleepUs/1000),(unsigned long)powerAudit.sleeps,(unsigned long)powerAudit.attempts,
            (unsigned long)powerAudit.failures,(unsigned long)powerAudit.redraws,(unsigned long)powerAudit.batteryReads,weatherService.standbyReady(),
            screenSleep.quarantined(),standbyInputPending(),unsigned(M5Cardputer.Keyboard.keyList().size()));
    }
    uiHealth().mark(UiPhase::Idle,millis());
    delay(screenOff?10:(e.changed?2:8));
}
