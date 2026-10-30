#include <cassert>
#include <cstring>
#include "../src/services/CryptoParser.h"
#include "../src/services/JsonDecode.h"
#include <string>
using namespace spark;
constexpr int64_t Now=1704067200;
int main() {
    assert(CoinCount==10 && !strcmp(Watchlist[0].id,"ripple") && !strcmp(Watchlist[9].id,"hyperliquid"));
    DynamicJsonDocument doc(8192);
    for(const auto& coin:Watchlist) {
        doc[coin.id]["usd"]=0.08335;
        doc[coin.id]["usd_24h_change"]=-2.5;
        doc[coin.id]["last_updated_at"]=Now;
    }
    { // Preserve all ten coins through the deferred, bounded decoder.
        DynamicJsonDocument filter(2048),deferred(0);
        for(const auto& coin:Watchlist){filter[coin.id]["usd"]=true;filter[coin.id]["usd_24h_change"]=true;filter[coin.id]["last_updated_at"]=true;}
        std::string body;serializeJson(doc,body);
        assert(deferred.capacity()==0 && !filter.overflowed());
        assert(!decodeJson(deferred,body.c_str(),filter.as<JsonVariantConst>(),8192));
        CryptoData previous,decoded;assert(parseCrypto(deferred.as<JsonVariantConst>(),Now,previous,decoded));
        for(const auto& price:decoded.prices)assert(price.valid && price.changeValid);
        assert(deferred.capacity()<=8192);
    }
    CryptoData empty, parsed;
    assert(parseCrypto(doc.as<JsonVariantConst>(),Now,empty,parsed));
    for(const auto& price:parsed.prices) assert(price.valid && price.changeValid && !price.retained);
    doc[Watchlist[0].id]["usd_24h_change"]=nullptr;
    CryptoData nullable;
    assert(parseCrypto(doc.as<JsonVariantConst>(),Now,parsed,nullable));
    assert(nullable.prices[0].valid && !nullable.prices[0].changeValid);
    doc[Watchlist[1].id]["usd"]=nullptr;
    CryptoData missing;
    assert(parseCrypto(doc.as<JsonVariantConst>(),Now,nullable,missing));
    assert(missing.prices[1].valid && missing.prices[1].retained);
    assert(coinStale(missing.prices[1],Now));
    doc[Watchlist[2].id]["last_updated_at"]=Now-1;
    assert(parseCrypto(doc.as<JsonVariantConst>(),Now,missing,parsed));
    assert(parsed.prices[2].updatedAt==Now && parsed.prices[2].retained);
    assert(!parseCrypto(doc.as<JsonVariantConst>(),0,parsed,missing));
    doc.clear(); assert(!parseCrypto(doc.as<JsonVariantConst>(),Now,parsed,missing));
    assert(!coinStale(nullable.prices[0],Now+899)); assert(coinStale(nullable.prices[0],Now+900));
    char value[32];
    for(double price:{0.,0.00000000123,0.007301,0.08335,1.37,1206.78,1e15}) {
        formatPrice(price,value,sizeof(value)); assert(strlen(value)<=9 && value[0]=='$');
    }
    formatPrice(-1,value,sizeof(value)); assert(!strcmp(value,"--"));
    formatPrice(NAN,value,sizeof(value)); assert(!strcmp(value,"--"));
    formatChange(-0.001,value,sizeof(value)); assert(!strcmp(value,"+0.0%"));
    formatChange(1e9,value,sizeof(value)); assert(strlen(value)<=9);
    CryptoSchedule schedule; assert(schedule.ready(0));
    schedule.finish(0,true,200); assert(!schedule.ready(599999)); assert(schedule.ready(600000));
    schedule.finish(600000,false,429,120); assert(!schedule.ready(719999)); assert(schedule.ready(720000));
    schedule.finish(720000,false,401); assert(!schedule.ready(10000000));
    schedule=CryptoSchedule{}; assert(schedule.ready(0));
    schedule.finish(0xffffff00,false,500); assert(!schedule.ready(0xffffff01));
    assert(schedule.ready(uint32_t(0xffffff00u+CryptoRetryMs)));
    CryptoNavigation nav; nav.move(-1); assert(nav.selected==9);
    nav.move(1); assert(nav.selected==0); nav.page(1); assert(nav.selected==3);
    nav.page(1); assert(nav.selected==6); nav.page(1); assert(nav.selected==9);
    nav.page(1); assert(nav.selected==0); nav.page(-1); assert(nav.selected==9);
}
