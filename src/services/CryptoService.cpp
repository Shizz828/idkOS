#include "CryptoService.h"
#include "CryptoParser.h"
#include "../core/NetworkMemory.h"
#include "JsonHttp.h"
#include <Preferences.h>
#include <stddef.h>
namespace spark {
static uint32_t hashBytes(const void* data,size_t length) {
    auto bytes=static_cast<const uint8_t*>(data); uint32_t hash=2166136261u;
    for(size_t i=0;i<length;++i) hash=(hash^bytes[i])*16777619u;
    return hash;
}
static uint32_t watchlistHash() {
    uint32_t hash=2166136261u;
    for(const auto& coin:Watchlist) {
        for(const char* c=coin.id;*c;++c) hash=(hash^uint8_t(*c))*16777619u;
        hash=(hash^0)*16777619u;
    }
    return hash;
}
struct SavedCrypto { uint32_t version=1, watchlist=0; CryptoData data; uint32_t checksum=0; };
bool CryptoService::begin() {
    mutex=xSemaphoreCreateMutex(); keys=xQueueCreate(1,sizeof(KeyCommand));
    if(!mutex || !keys) {
        if(mutex) vSemaphoreDelete(mutex); if(keys) vQueueDelete(keys);
        mutex=nullptr; keys=nullptr; return false;
    }
    // Restore the saved key and cache before the network worker starts, so
    // Crypto never shows the provisioning prompt for a key that is stored.
    load();
    return true;
}
bool CryptoService::snapshot(CryptoSnapshot& output) {
    if(!mutex || xSemaphoreTake(mutex,0)!=pdTRUE) return false;
    output=published; xSemaphoreGive(mutex); return true;
}
bool CryptoService::configureKey(const char* key) {
    if(!keys || !key || strncmp(key,"CG-",3) || strlen(key)<=3 || strlen(key)>=sizeof(apiKey)) return false;
    for(const char* c=key;*c;++c) if(uint8_t(*c)<=32 || uint8_t(*c)>126) return false;
    KeyCommand command{}; strlcpy(command.key,key,sizeof(command.key));
    bool ok=xQueueSend(keys,&command,0)==pdTRUE; memset(&command,0,sizeof(command)); return ok;
}
void CryptoService::publish() {
    if(!mutex) return;
    xSemaphoreTake(mutex,portMAX_DELAY);
    working.revision=published.revision+1; published=working;
    xSemaphoreGive(mutex);
}
void CryptoService::load() {
    Preferences prefs;
    if(prefs.begin("spark-crypto",true)) {
        prefs.getString("api-key",apiKey,sizeof(apiKey));
        SavedCrypto saved{};
        bool ok=prefs.getBytesLength("cache")==sizeof(saved) && prefs.getBytes("cache",&saved,sizeof(saved))==sizeof(saved);
        if(ok && saved.version==1 && saved.watchlist==watchlistHash() && saved.checksum==hashBytes(&saved,offsetof(SavedCrypto,checksum))) {
            bool valid=true;
            for(const auto& coin:saved.data.prices)
                if(coin.valid && (!isfinite(coin.price) || coin.price<0 || coin.price>1e15 || coin.updatedAt<CryptoClockMinimum)) valid=false;
            if(valid) { working.data=saved.data; working.fromDisk=saved.data.valid; }
        }
        prefs.end();
    }
    working.keyConfigured=apiKey[0]!=0;
    snprintf(working.status,sizeof(working.status),working.keyConfigured?"Waiting for Wi-Fi / time":"Use USB to set Demo key");
    loaded=true; publish();
}
bool CryptoService::saveCache() {
    SavedCrypto saved{}; saved.watchlist=watchlistHash(); saved.data=working.data;
    saved.checksum=hashBytes(&saved,offsetof(SavedCrypto,checksum));
    Preferences prefs; if(!prefs.begin("spark-crypto",false)) return false;
    bool ok=prefs.putBytes("cache",&saved,sizeof(saved))==sizeof(saved); prefs.end(); return ok;
}
void CryptoService::poll(bool online,int64_t now) {
    if(!mutex || !keys) return;
    if(!loaded) load();
    KeyCommand command{};
    if(xQueueReceive(keys,&command,0)==pdTRUE) {
        Preferences prefs;
        bool saved=false;
        if(prefs.begin("spark-crypto",false)) {
            saved=prefs.putString("api-key",command.key)==strlen(command.key);
            prefs.end();
        }
        if(saved) {
            strlcpy(apiKey,command.key,sizeof(apiKey)); working.keyConfigured=true;
            working.failed=false; schedule=CryptoSchedule{};
            snprintf(working.status,sizeof(working.status),"Demo key saved; waiting for data");
            Serial.println("IDKOS_CRYPTO_KEY_SAVED");
        } else {
            snprintf(working.status,sizeof(working.status),"Key save failed");
            Serial.println("IDKOS_CRYPTO_KEY_SAVE_FAILED");
        }
        memset(&command,0,sizeof(command)); publish();
    }
    if(working.online!=online) { working.online=online; publish(); }
    if(!online || !working.keyConfigured || now<CryptoClockMinimum || !schedule.ready(millis())) return;
    HttpMemoryLease memory;
    if(!memory)return; // Defer before allocating JSON/TLS buffers for SSH startup.
    working.busy=true; snprintf(working.status,sizeof(working.status),"Updating / CoinGecko"); publish();
    String url="https://api.coingecko.com/api/v3/simple/price?ids=";
    for(unsigned i=0;i<CoinCount;++i) { if(i) url+=','; url+=Watchlist[i].id; }
    url+="&vs_currencies=usd&include_24hr_change=true&include_last_updated_at=true";
    DynamicJsonDocument filter(2048), doc(0);
    HttpResult result;
    bool ok=false;
    if(filter.capacity()==0) snprintf(working.status,sizeof(working.status),"Crypto memory low");
    else {
        for(const auto& coin:Watchlist) {
            filter[coin.id]["usd"]=true; filter[coin.id]["usd_24h_change"]=true; filter[coin.id]["last_updated_at"]=true;
        }
        result=fetchJson(url.c_str(),doc,filter.as<JsonVariantConst>(),working.status,sizeof(working.status),8192,"x-cg-demo-api-key",apiKey,8192);
        CryptoData next;
        if(result.ok) {
            ok=parseCrypto(doc.as<JsonVariantConst>(),now,working.data,next);
            if(ok) {
                working.data=next; working.fromDisk=false; working.cacheWriteFailed=!saveCache();
                bool partial=false; for(const auto& p:next.prices) if(!p.valid || p.retained) partial=true;
                snprintf(working.status,sizeof(working.status),working.cacheWriteFailed?"Cache save failed":partial?"Partial data / CoinGecko":"Data provided by CoinGecko");
            } else snprintf(working.status,sizeof(working.status),"Invalid price data / cached");
        } else if(result.code==401 || result.code==403) snprintf(working.status,sizeof(working.status),"Demo key rejected; replace via USB");
        else if(result.code==429) snprintf(working.status,sizeof(working.status),"Rate limited / cached");
    }
    schedule.finish(millis(),ok,result.code,result.retryAfterSeconds);
    working.failed=!ok; working.busy=false; publish();
}
}
