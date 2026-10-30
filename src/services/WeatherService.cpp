#include "WeatherService.h"
#include "WeatherParser.h"
#include "JsonHttp.h"
#include "CryptoService.h"
#include "KnownNetworks.h"
#include "../core/NetworkMemory.h"
#include "../core/WifiReconnect.h"
#include <WiFi.h>
#include <Preferences.h>
#include <time.h>
#include <stddef.h>
#include <atomic>
#include <esp_sntp.h>
#include <esp_wifi.h>
#include "../core/SystemTime.h"
#include "../core/PowerPolicy.h"
#include "../core/UiHealth.h"
namespace spark {
static std::atomic<uint8_t> disconnectReason{0};
static const char* ForecastSuffix=
    "&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,wind_speed_10m,wind_direction_10m"
    "&hourly=temperature_2m,weather_code&daily=temperature_2m_max,temperature_2m_min"
    "&temperature_unit=fahrenheit&wind_speed_unit=mph&timeformat=unixtime"
    "&timezone=auto&forecast_days=2";

static WeatherFault faultFor(const HttpResult& result,int16_t& code) {
    code=0;
    switch(result.failure) {
    case HttpFailure::Connection: return WeatherFault::Https;
    case HttpFailure::Tls: code=int16_t(result.tlsError); return WeatherFault::Https;
    case HttpFailure::Http: code=int16_t(result.code); return WeatherFault::Http;
    default: return WeatherFault::Data;
    }
}
static bool getForecast(const WeatherLocation& location,int64_t now, WeatherData& output, char* error, size_t errorSize, WeatherFault& fault, int16_t& faultCode) {
    DynamicJsonDocument filter(2048), doc(0);
    fault=WeatherFault::None; faultCode=0;
    if(filter.capacity()==0) { snprintf(error,errorSize,"Weather memory low"); fault=WeatherFault::Memory; return false; }
    filter["utc_offset_seconds"]=true;
    filter["current"]=true; filter["current_units"]=true;
    filter["daily"]["temperature_2m_max"]=true; filter["daily"]["temperature_2m_min"]=true;
    filter["hourly"]["time"]=true; filter["hourly"]["temperature_2m"]=true; filter["hourly"]["weather_code"]=true;
    char coords[96]; snprintf(coords,sizeof(coords),"https://api.open-meteo.com/v1/forecast?latitude=%.5f&longitude=%.5f",location.latitude,location.longitude);
    String url=String(coords)+ForecastSuffix;
    HttpResult result=fetchJson(url.c_str(),doc,filter.as<JsonVariantConst>(),error,errorSize,49152,nullptr,nullptr,16384);
    if(!result.ok) { fault=faultFor(result,faultCode); return false; }
    if(!parseWeather(doc.as<JsonVariantConst>(),now,output)) { snprintf(error,errorSize,"Weather fields invalid"); fault=WeatherFault::Data; return false; }
    return true;
}
static bool getAlerts(const WeatherLocation& location,int64_t now, AlertData& output, char* error, size_t errorSize) {
    DynamicJsonDocument filter(512), doc(0);
    if(filter.capacity()==0) { snprintf(error,errorSize,"Alert memory low"); return false; }
    filter["type"]=true;
    auto p=filter["features"][0]["properties"];
    for(const char* name:{"event","expires","effective","severity","status","messageType"}) p[name]=true;
    char url[128]; snprintf(url,sizeof(url),"https://api.weather.gov/alerts/active?point=%.5f,%.5f",location.latitude,location.longitude);
    if(!fetchJson(url,doc,filter.as<JsonVariantConst>(),error,errorSize,49152,nullptr,nullptr,24576).ok) return false;
    if(!parseAlerts(doc.as<JsonVariantConst>(),now,output)) { snprintf(error,errorSize,"Alert fields invalid"); return false; }
    return true;
}
struct SavedWeather {
    uint32_t version=2;
    WeatherLocation location;
    WeatherData weather;
    AlertData alerts;
    uint32_t checksum=0;
};
static uint32_t checksum(const SavedWeather& data) {
    uint32_t result=2166136261u;
    auto bytes=reinterpret_cast<const uint8_t*>(&data);
    for(size_t i=0;i<offsetof(SavedWeather,checksum);++i) result=(result^bytes[i])*16777619u;
    return result;
}
struct LegacyWeatherData {
    bool valid=false, dailyValid=false;
    float temperature=0, feelsLike=0, high=0, low=0, windSpeed=0;
    int16_t code=-1, humidity=0, windDirection=0;
    int64_t updatedAt=0, observedAt=0;
    WeatherHour hours[4];
};
struct LegacySavedWeather { uint32_t version=1; LegacyWeatherData weather; AlertData alerts; uint32_t checksum=0; };
static bool loadCache(WeatherSnapshot& s) {
    Preferences prefs;
    if(!prefs.begin("spark-weather",true)) return false;
    if(prefs.getBytesLength("cache")==sizeof(LegacySavedWeather)) {
        LegacySavedWeather legacy{}; bool read=prefs.getBytes("cache",&legacy,sizeof(legacy))==sizeof(legacy);
        uint32_t hash=2166136261u; auto bytes=reinterpret_cast<const uint8_t*>(&legacy);
        for(size_t i=0;i<offsetof(LegacySavedWeather,checksum);++i) hash=(hash^bytes[i])*16777619u;
        if(read && legacy.version==1 && hash==legacy.checksum) {
            memcpy(&s.weather,&legacy.weather,sizeof(legacy.weather)); s.weather.utcOffset=INT32_MIN;
            s.alerts=legacy.alerts; s.fromDisk=s.weather.valid; prefs.end(); return true;
        }
    }
    SavedWeather saved{};
    bool ok=prefs.getBytesLength("cache")==sizeof(saved) && prefs.getBytes("cache",&saved,sizeof(saved))==sizeof(saved);
    prefs.end();
    if(!ok || saved.version!=2 || saved.checksum!=checksum(saved) || !validLocation(saved.location)) return false;
    if(saved.weather.valid && (!isfinite(saved.weather.temperature) || saved.weather.updatedAt<ClockMinimum)) return false;
    if(saved.alerts.headline[sizeof(saved.alerts.headline)-1]!=0) return false;
    s.weather=saved.weather; s.alerts=saved.alerts; s.location=saved.location;
    s.fromDisk=s.weather.valid; return true;
}
static bool saveCache(const WeatherSnapshot& s) {
    SavedWeather saved{}; saved.location=s.location; saved.weather=s.weather; saved.alerts=s.alerts; saved.checksum=checksum(saved);
    Preferences prefs;
    if(!prefs.begin("spark-weather",false)) return false;
    bool ok=prefs.putBytes("cache",&saved,sizeof(saved))==sizeof(saved);
    prefs.end(); return ok;
}
static LocationConfig loadLocations() {
    LocationConfig config; Preferences p;
    if(p.begin("spark-location",true)) {
        LocationConfig stored;
        if(p.getBytesLength("config")==sizeof(stored) && p.getBytes("config",&stored,sizeof(stored))==sizeof(stored) &&
           stored.version==1 && unsigned(stored.mode)<=2 && validLocation(stored.home) && validLocation(stored.manual) && validLocation(stored.last)) config=stored;
        p.end();
    }
    return config;
}
static void saveLocations(const LocationConfig& config) { Preferences p; if(p.begin("spark-location",false)) { p.putBytes("config",&config,sizeof(config)); p.end(); } }
static bool autoLocation(WeatherLocation& location) {
    DynamicJsonDocument filter(512),doc(0); char error[40];
    for(const char* key:{"city","region_code","country_code","latitude","longitude","error"}) filter[key]=true;
    if(!fetchJson("https://ipapi.co/json/",doc,filter.as<JsonVariantConst>(),error,sizeof(error),4096,nullptr,nullptr,2048).ok || doc["error"]==true ||
       !doc["latitude"].is<double>() || !doc["longitude"].is<double>() || !doc["city"].is<const char*>()) return false;
    WeatherLocation next;
    next.latitude=doc["latitude"].as<double>(); next.longitude=doc["longitude"].as<double>();
    char label[64]; snprintf(label,sizeof(label),"%s, %s",doc["city"]|"",doc["region_code"]|"");
    compactText(label,next.label,sizeof(next.label)); strlcpy(next.country,doc["country_code"]|"",sizeof(next.country)); next.approximate=true;
    if(!validLocation(next)) return false;
    location=next; return true;
}
bool WeatherService::configureLocation(LocationMode mode,const WeatherLocation& location,bool saveHome) {
    if(!commands || !validLocation(location)) return false;
    Credentials command{}; command.locationCommand=true; command.mode=mode; command.location=location; command.saveHome=saveHome;
    return xQueueSend(commands,&command,0)==pdTRUE;
}
bool WeatherService::begin(CryptoService* crypto) {
    market=crypto;
    mutex=xSemaphoreCreateMutex(); commands=xQueueCreate(1,sizeof(Credentials));
    if(!mutex || !commands) {
        if(mutex) vSemaphoreDelete(mutex);
        if(commands) vQueueDelete(commands);
        mutex=nullptr; commands=nullptr; return false;
    }
    if(xTaskCreatePinnedToCore(taskEntry,"idkos-weather",16384,this,1,nullptr,0)!=pdPASS) {
        vSemaphoreDelete(mutex); vQueueDelete(commands); mutex=nullptr; commands=nullptr; return false;
    }
    return true;
}
bool WeatherService::snapshot(WeatherSnapshot& out) {
    if(!mutex || xSemaphoreTake(mutex,0)!=pdTRUE) return false;
    out=published; xSemaphoreGive(mutex); return true;
}
uint32_t WeatherService::connect(const char* ssid,const char* password) {
    if(!commands || !ssid[0] || strlen(ssid)>32 || strlen(password)>64) return false;
    Credentials command{}; strlcpy(command.ssid,ssid,sizeof(command.ssid)); strlcpy(command.password,password,sizeof(command.password));
    command.attempt=nextAttempt+1;
    if(!command.attempt) command.attempt=1;
    bool ok=xQueueSend(commands,&command,0)==pdTRUE;
    if(ok) nextAttempt=command.attempt;
    memset(&command,0,sizeof(command)); return ok ? nextAttempt : 0;
}
bool WeatherService::networkAction(uint8_t action,unsigned index) {
    if(!commands || action<1 || action>4 || index>=5) return false;
    Credentials command{};command.networkAction=action;command.networkIndex=index;
    return xQueueSend(commands,&command,0)==pdTRUE;
}
void WeatherService::publish(WeatherSnapshot& state) {
    xSemaphoreTake(mutex,portMAX_DELAY);
    state.revision=published.revision+1; published=state;
    xSemaphoreGive(mutex);
}
void WeatherService::taskEntry(void* context) { static_cast<WeatherService*>(context)->run(); }
void WeatherService::run() {
    WeatherSnapshot state;
    LocationConfig locations=loadLocations();
    WeatherLocation target=locations.mode==LocationMode::Auto?locations.last:locations.mode==LocationMode::Home?locations.home:locations.manual;
    state.locationMode=locations.mode;
    KnownNetworks networks; networks.begin(); state.knownNetworks=networks.count(); networks.names(state.knownSsids);
    Credentials accepted; bool rememberPending=false;
    loadCache(state); publish(state);
    WiFi.onEvent([](WiFiEvent_t event,WiFiEventInfo_t info) {
        if(event==ARDUINO_EVENT_WIFI_STA_DISCONNECTED && info.wifi_sta_disconnected.reason!=8)
            disconnectReason.store(info.wifi_sta_disconnected.reason);
    });
    WiFi.mode(WIFI_STA); WiFi.setAutoReconnect(false); WiFi.persistent(false);
    esp_err_t ps=esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    Serial.printf("[power] wifi modem sleep result=%d\n",int(ps));
    state.wifi.start(0,millis()); publish(state);
    char initialSsid[33]{},initialPassword[65]{};
    // Prefer a saved network that is actually in range instead of spending a
    // timeout on each unavailable profile from a previous location.
    int found=networks.count()?WiFi.scanNetworks(false,true):0;
    bool matched=false;
    for(int i=0;i<found && !matched;++i)
        matched=networks.matching(WiFi.SSID(i).c_str(),initialSsid,initialPassword);
    WiFi.scanDelete();
    if(matched || networks.candidate(initialSsid,initialPassword)) WiFi.begin(initialSsid,initialPassword);
    else WiFi.begin(); // Legacy driver credentials remain a fallback.
    memset(initialPassword,0,sizeof(initialPassword));
    sntp_set_time_sync_notification_cb([](struct timeval*) { systemTimeSynchronized().store(true); });
    configTzTime("EST5EDT,M3.2.0/2,M11.1.0/2","pool.ntp.org","time.nist.gov");
    RefreshSchedule forecast, alerts;
    uint32_t lastReconnect=millis(),geoDue=0; String geoNetwork;
    bool radioStopped=false;
    WifiRetryPolicy retry;
    while(true) {
        // Reuse the existing worker to report a blocked UI; no polling task,
        // flash writes or automatic reboot (which could discard a draft).
        uiHealth().poll(millis(),[](const char* phase,uint32_t elapsed){
            Serial.printf("[ui] stalled phase=%s ms=%lu heap=%u\n",phase,(unsigned long)elapsed,ESP.getFreeHeap());
        });
        // Cooperatively pause here, never suspend a task inside TLS/NVS/I2C.
        // This worker also owns Crypto polling, so its acknowledgement covers both.
        uint32_t standbyTicket=standby.ticket();
        if(NetworkStandby::pausing(standbyTicket)) {
            if(!radioStopped) {
                radioStopped=esp_wifi_stop()==ESP_OK;
                if(radioStopped){state.online=false;state.busy=false;publish(state);}
            }
            if(radioStopped)standby.acknowledge(standbyTicket);
            vTaskDelay(pdMS_TO_TICKS(50));continue;
        }
        if(radioStopped) {
            // esp_wifi_stop retains STA configuration and saved credentials.
            if(esp_wifi_start()!=ESP_OK){vTaskDelay(pdMS_TO_TICKS(250));continue;}
            radioStopped=false;disconnectReason.store(0);
            state.wifi.start(rememberPending?accepted.attempt:0,millis());lastReconnect=millis();
            retry.reset();
            esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
            // Reconnect from saved profiles (or a pending Settings request)
            // instead of relying only on the driver's retained configuration.
            char ssid[33]{},password[65]{};
            if(wifiReconnectCredentials(networks,WiFi.SSID().c_str(),rememberPending,accepted.ssid,accepted.password,ssid,password))
                WiFi.begin(ssid,password);
            else esp_wifi_connect();
            memset(password,0,sizeof(password));
        }
        Credentials command{};
        bool received=xQueueReceive(commands,&command,0)==pdTRUE;
        if(received && command.networkAction) {
            if(command.networkAction!=4)retry.reset();
            if(command.networkAction==4) {
                state.scanning=true;state.scanFailed=false;state.nearbyCount=0;publish(state);
                // The existing network worker owns all scans, including startup.
                // Do not disconnect or change credentials to scan.
                int count=WiFi.scanNetworks(false,true);
                state.scanFailed=count<0;
                for(int i=0;i<count && state.nearbyCount<12;++i) {
                    String name=WiFi.SSID(i);
                    if(!name.length())continue; // Hidden SSIDs use manual entry.
                    bool duplicate=false;
                    for(unsigned j=0;j<state.nearbyCount;++j)
                        if(name==state.nearby[j].ssid)duplicate=true;
                    if(duplicate)continue;
                    auto& entry=state.nearby[state.nearbyCount++];
                    strlcpy(entry.ssid,name.c_str(),sizeof(entry.ssid));entry.rssi=WiFi.RSSI(i);
                }
                WiFi.scanDelete();state.scanning=false;++state.scanRevision;publish(state);
            } else if(command.networkAction==2) {
                char forgotten[33]{},secret[65]{};
                bool found=networks.selected(command.networkIndex,forgotten,secret);memset(secret,0,sizeof(secret));
                bool ok=found && networks.forget(command.networkIndex);
                if(ok && WiFi.SSID()==forgotten) {WiFi.disconnect(false,true);lastReconnect=millis();rememberPending=false;memset(accepted.password,0,sizeof(accepted.password));}
                state.networkSaveFailed=!ok;state.knownNetworks=networks.count();networks.names(state.knownSsids);
                snprintf(state.status,sizeof(state.status),ok?"Network forgotten":"Forget failed; retry");publish(state);
            } else {
                bool found=command.networkAction==1?networks.selected(command.networkIndex,command.ssid,command.password):
                    (networks.matching(WiFi.SSID().c_str(),command.ssid,command.password) || networks.candidate(command.ssid,command.password));
                if(found) {
                    WiFi.disconnect(false,false);disconnectReason.store(0);state.wifi.start(0,millis());
                    rememberPending=false;memset(accepted.password,0,sizeof(accepted.password));WiFi.begin(command.ssid,command.password);lastReconnect=millis();
                } else if(command.networkAction==3) {WiFi.reconnect();lastReconnect=millis();}
                snprintf(state.status,sizeof(state.status),found || command.networkAction==3?"Reconnecting Wi-Fi":"Saved network unavailable");publish(state);
            }
            memset(&command,0,sizeof(command));
        }
        if(received && !command.networkAction && command.locationCommand) {
            locations.mode=command.mode;
            if(command.mode==LocationMode::Manual) locations.manual=command.location;
            if(command.saveHome) { locations.home=command.location; locations.mode=LocationMode::Home; }
            target=locations.mode==LocationMode::Auto?locations.last:locations.mode==LocationMode::Home?locations.home:locations.manual;
            state.locationMode=locations.mode; saveLocations(locations); geoDue=0; forecast=RefreshSchedule{}; alerts=RefreshSchedule{}; publish(state);
        } else if(!command.networkAction && command.ssid[0]) {
            retry.reset();
            accepted=command; rememberPending=true;
            WiFi.disconnect(false,false);
            disconnectReason.store(0);
            state.wifi.start(command.attempt,millis()); state.online=false;
            snprintf(state.status,sizeof(state.status),"Connecting Wi-Fi"); publish(state);
            WiFi.begin(command.ssid,command.password);
            memset(&command,0,sizeof(command));
            lastReconnect=millis();
            snprintf(state.status,sizeof(state.status),"Connecting Wi-Fi"); publish(state);
        }
        bool online=WiFi.status()==WL_CONNECTED;
        // Save accepted Settings credentials whenever the station is connected
        // to that SSID; a quick reconnect can complete between worker polls,
        // so this must not depend on observing an offline->online edge.
        if(online && rememberPending && WiFi.SSID()==accepted.ssid) {
            state.networkSaveFailed=!networks.remember(accepted.ssid,accepted.password);
            memset(accepted.password,0,sizeof(accepted.password));rememberPending=false;
            state.knownNetworks=networks.count();networks.names(state.knownSsids);publish(state);
        }
        bool connectionChanged=state.wifi.update(online,disconnectReason.load(),millis());
        if(online!=state.online || connectionChanged) {
            if(state.online && !online){retry.reset();lastReconnect=millis()-15000;}
            if(restartTimeSync(state.online,online,systemTimeSynchronized().load()))sntp_restart();
            state.online=online;
            if(online) {
                retry.reset();
                disconnectReason.store(0);
            }
            snprintf(state.status,sizeof(state.status),online ? "Synchronizing clock" : "Offline; Enter for Wi-Fi");
            publish(state);
        }
        uint32_t ms=millis(); int64_t now=time(nullptr);
        if(market) market->poll(online,now);
        if(!online) {
            uint32_t elapsed=uint32_t(ms-lastReconnect);
            uint32_t cooldown=retry.waitMs(networks.count());
            if(elapsed>=cooldown && (state.wifi.phase!=WifiPhase::Connecting || elapsed>=20000)) {
                disconnectReason.store(0);
                // Keep a Settings request until it connects and is saved, or the
                // user explicitly selects another network or forgets it.
                state.wifi.start(rememberPending?accepted.attempt:0,ms);
                char ssid[33]{},password[65]{};
                WiFi.disconnect(false,false);
                if(wifiReconnectCredentials(networks,"",rememberPending,accepted.ssid,accepted.password,ssid,password)) WiFi.begin(ssid,password); else WiFi.begin();
                memset(password,0,sizeof(password)); lastReconnect=ms;retry.attempted();
                Serial.printf("[power] wifi retry next_cooldown_ms=%lu\n",(unsigned long)retry.waitMs(networks.count()));
                snprintf(state.status,sizeof(state.status),"Offline; Enter for Wi-Fi"); publish(state);
            }
        } else if(now<ClockMinimum) {
            // Never bypass TLS verification when the clock is unavailable.
            if(strcmp(state.status,"Waiting for network time")) {
                snprintf(state.status,sizeof(state.status),"Waiting for network time"); publish(state);
            }
        } else {
            if(locations.mode==LocationMode::Auto && (WiFi.SSID()!=geoNetwork || int32_t(ms-geoDue)>=0)) {
                HttpMemoryLease memory;
                if(memory){
                    geoNetwork=WiFi.SSID(); geoDue=ms+21600000;
                    WeatherLocation found;
                    if(autoLocation(found)) {
                        if(!sameLocation(target,found)) { forecast=RefreshSchedule{}; alerts=RefreshSchedule{}; }
                        target=locations.last=found; saveLocations(locations);
                    }
                }
            }
            if(forecast.ready(millis())) {
                HttpMemoryLease memory;
                if(memory){
                    state.busy=true; snprintf(state.status,sizeof(state.status),"Updating weather"); publish(state);
                    WeatherData next;
                    bool ok=getForecast(target,time(nullptr),next,state.status,sizeof(state.status),state.weatherFault,state.weatherFaultCode);
                    forecast.finish(millis(),ok); state.weatherFailed=!ok;
                    if(ok) { if(!sameLocation(state.location,target)) state.alerts=AlertData{}; state.location=target; state.weather=next; state.fromDisk=false; state.cacheWriteFailed=!saveCache(state); snprintf(state.status,sizeof(state.status),"Weather updated"); }
                    state.busy=false; publish(state);
                }
            }
            if(WiFi.status()==WL_CONNECTED && alerts.ready(millis())) {
                HttpMemoryLease memory;
                if(memory){
                    state.busy=true; publish(state);
                    AlertData next;
                    bool ok=!strcmp(state.location.country,"US") && getAlerts(state.location,time(nullptr),next,state.status,sizeof(state.status));
                    alerts.finish(millis(),ok); state.alertsFailed=!ok;
                    if(ok) { state.alerts=next; state.cacheWriteFailed=!saveCache(state); snprintf(state.status,sizeof(state.status),"Alerts updated"); }
                    state.busy=false; publish(state);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
}
