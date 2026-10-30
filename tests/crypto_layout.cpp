#include <initializer_list>
#include "../src/apps/CryptoApp.h"
int main() {
    spark::CryptoSnapshot state; spark::CryptoNavigation nav;
    spark::drawCrypto(state,nav,0);
    state.online=true; state.keyConfigured=true; state.data.valid=true;
    for(auto& price:state.data.prices) {
        price.valid=true; price.changeValid=true; price.updatedAt=1704067200;
    }
    for(double value:{0.,0.00000000123,0.007301,0.08335,1.37,1206.78,1e15}) {
        for(auto& price:state.data.prices) { price.price=value; price.change=1e9; }
        for(unsigned i=0;i<spark::CoinCount;++i) {
            nav.selected=i; spark::drawCrypto(state,nav,1704067200);
        }
    }
    state.fromDisk=true; state.online=false; spark::drawCrypto(state,nav,0);
}
