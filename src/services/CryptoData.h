#pragma once
#include "Watchlist.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
namespace spark {
constexpr uint32_t CryptoRefreshMs=600000, CryptoRetryMs=60000;
constexpr int64_t CryptoClockMinimum=1704067200;
struct CoinPrice {
    double price=0, change=0;
    int64_t updatedAt=0;
    bool valid=false, changeValid=false, retained=false;
};
struct CryptoData {
    CoinPrice prices[CoinCount];
    int64_t fetchedAt=0;
    bool valid=false;
};
struct CryptoSnapshot {
    CryptoData data;
    bool online=false, busy=false, keyConfigured=false, fromDisk=false;
    bool failed=false, cacheWriteFailed=false;
    char status[40]="Use USB to set Demo key";
    uint32_t revision=0;
};
inline bool coinStale(const CoinPrice& price,int64_t now) {
    return price.retained || now<CryptoClockMinimum || price.updatedAt<CryptoClockMinimum ||
           now<price.updatedAt || now-price.updatedAt>=900;
}
inline void formatPrice(double price,char* out,size_t capacity) {
    if(!isfinite(price) || price<0) { snprintf(out,capacity,"--"); return; }
    if(price==0 || price>=1) snprintf(out,capacity,"$%.2f",price);
    else snprintf(out,capacity,"$%.5g",price);
    if(strlen(out)>9) snprintf(out,capacity,"$%.4g",price);
}
inline void formatChange(double change,char* out,size_t capacity) {
    if(!isfinite(change)) { snprintf(out,capacity,"--"); return; }
    if(fabs(change)<0.05) change=0;
    snprintf(out,capacity,"%+.1f%%",change);
    if(strlen(out)>9) snprintf(out,capacity,"%+.2g%%",change);
}
struct CryptoSchedule {
    bool scheduled=false, authBlocked=false;
    uint32_t due=0, backoff=CryptoRetryMs;
    bool ready(uint32_t now) const { return !authBlocked && (!scheduled || int32_t(now-due)>=0); }
    void finish(uint32_t now,bool success,int code,uint32_t retrySeconds=0) {
        scheduled=true;
        if(code==401 || code==403) { authBlocked=true; return; }
        uint32_t delay=CryptoRefreshMs;
        if(success) backoff=CryptoRetryMs;
        else {
            delay=backoff;
            if(code==429) {
                uint32_t requested=retrySeconds>3600 ? 3600000 : retrySeconds*1000;
                if(requested>delay) delay=requested;
            }
            backoff=backoff>=1800000 ? 3600000 : backoff*2;
        }
        due=now+delay;
    }
};
constexpr unsigned CryptoRows=3;
constexpr unsigned CryptoPages=(CoinCount+CryptoRows-1)/CryptoRows;
struct CryptoNavigation {
    unsigned selected=0;
    void move(int delta) { selected=(selected+CoinCount+delta)%CoinCount; }
    void page(int delta) {
        unsigned p=(selected/CryptoRows+CryptoPages+delta)%CryptoPages;
        selected=p*CryptoRows;
    }
};
}
