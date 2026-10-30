#pragma once
#include <ArduinoJson.h>
#include "CryptoData.h"
namespace spark {
inline bool parseCrypto(JsonVariantConst root,int64_t now,const CryptoData& previous,CryptoData& output) {
    if(!root.is<JsonObjectConst>() || now<CryptoClockMinimum) return false;
    CryptoData next=previous;
    unsigned valid=0;
    for(unsigned i=0;i<CoinCount;++i) {
        auto value=root[Watchlist[i].id]; auto& price=next.prices[i];
        price.retained=price.valid;
        // Never preserve an old percentage alongside a newly fetched price.
        if(!value["usd"].is<double>() || !value["last_updated_at"].is<int64_t>()) continue;
        double usd=value["usd"].as<double>(); int64_t updated=value["last_updated_at"].as<int64_t>();
        if(!isfinite(usd) || usd<0 || usd>1e15 || updated<CryptoClockMinimum || updated>now+300) continue;
        // An out-of-order cached API response must not replace newer data.
        if(price.valid && updated<price.updatedAt) continue;
        price.price=usd; price.updatedAt=updated; price.valid=true; price.retained=false;
        price.change=0; price.changeValid=false;
        if(value["usd_24h_change"].is<double>()) {
            double change=value["usd_24h_change"].as<double>();
            if(isfinite(change) && change>=-100 && change<=1e9) { price.change=change; price.changeValid=true; }
        }
        ++valid;
    }
    if(!valid) return false;
    next.valid=true; next.fetchedAt=now; output=next; return true;
}
}
