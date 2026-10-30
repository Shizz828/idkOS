#pragma once
#include "../services/CryptoData.h"
#include "../ui/Screen.h"
namespace spark {
inline void drawCrypto(const CryptoSnapshot& state,const CryptoNavigation& nav,int64_t now) {
    frame("CRYPTO");
    text(145,17,"USD / 24H",1,theme::Primary);
    auto& d=screen();
    unsigned start=nav.selected/CryptoRows*CryptoRows;
    for(unsigned row=0;row<CryptoRows && start+row<CoinCount;++row) {
        unsigned i=start+row; int y=30+row*27;
        const auto& price=state.data.prices[i];
        char symbol[9]{}; strlcpy(symbol,Watchlist[i].symbol,sizeof(symbol));
        if(strlen(Watchlist[i].symbol)>8) symbol[7]='~';
        text(6,y,symbol,strlen(symbol)<=4?2:1);
        char value[32];
        if(price.valid) formatPrice(price.price,value,sizeof(value)); else snprintf(value,sizeof(value),"--");
        text(57,y,value,strlen(value)<=9?2:1);
        if(price.valid && price.changeValid) formatChange(price.change,value,sizeof(value)); else snprintf(value,sizeof(value),"--");
        uint16_t color=price.changeValid && price.change!=0 ? (price.change>0 ? theme::Success : theme::Error) : theme::Content;
        text(174,y+2,value,1,color);
        bool old=state.fromDisk || !state.online || state.failed || coinStale(price,now);
        text(57,y+18,!price.valid?"No data":old?"Cached / stale":"Updated",1,old?theme::Attention:theme::Success);
        if(i==nav.selected) d.drawFastVLine(1,y,24,theme::Content);
        d.drawFastHLine(6,y+25,228,theme::Primary);
    }
    char status[38]{};
    if(!state.keyConfigured) strlcpy(status,state.status,sizeof(status));
    else if(!state.online) snprintf(status,sizeof(status),"Offline / cached | CoinGecko");
    else { strlcpy(status,state.status,sizeof(status)); }
    text(6,114,status,1,state.failed?theme::Error:!state.keyConfigured?theme::Attention:theme::Content);
    char hint[40]; snprintf(hint,sizeof(hint),"%u/%u arrows  Esc home",nav.selected/CryptoRows+1,CryptoPages);
    text(6,126,hint);
}
}
