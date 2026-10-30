#include <cassert>
#include <cstring>
#include "../src/services/KnownNetworks.h"
#include "../src/core/WifiReconnect.h"
int main(){using namespace spark;
    Preferences::data().clear();KnownNetworks networks;networks.begin();
    assert(networks.remember("older-network","older-password"));
    char ssid[33]{},password[65]{};
    // A slow Settings connection must win over saved profiles on every retry
    // and a radio restart, even when the driver reports an older current SSID.
    for(unsigned retry=0;retry<12;++retry){
        assert(wifiReconnectCredentials(networks,retry%2?"older-network":"",true,
            "new-network","new-password",ssid,password));
        assert(!strcmp(ssid,"new-network") && !strcmp(password,"new-password"));
    }
    // Offline retries do not persist an unaccepted password or alter old profiles.
    assert(networks.count()==1);
    KnownNetworks beforeAcceptance;beforeAcceptance.begin();assert(beforeAcceptance.count()==1);
    assert(!beforeAcceptance.matching("new-network",ssid,password));
    // Once accepted and saved, runtime pending state is no longer required.
    assert(networks.remember("new-network","new-password"));
    for(unsigned boot=0;boot<3;++boot){
        KnownNetworks restored;restored.begin();
        assert(restored.count()==2);
        assert(wifiReconnectCredentials(restored,"new-network",false,"","",ssid,password));
        assert(!strcmp(ssid,"new-network") && !strcmp(password,"new-password"));
        // Simulate lost runtime driver config on wake: pick a durable profile.
        assert(wifiReconnectCredentials(restored,"",false,"","",ssid,password));
        assert(!strcmp(ssid,"new-network") && !strcmp(password,"new-password"));
    }
    // An explicit replacement uses only the newly selected Settings credentials.
    assert(wifiReconnectCredentials(networks,"new-network",true,
        "replacement","replacement-password",ssid,password));
    assert(!strcmp(ssid,"replacement") && !strcmp(password,"replacement-password"));
    // Open networks must retain an empty password (never reuse the previous one).
    assert(wifiReconnectCredentials(networks,"",true,"open-network","",ssid,password));
    assert(!strcmp(ssid,"open-network") && !password[0]);
    // A failed durable write must remain a failure, preserving old profiles.
    Preferences::writesFail()=true;
    assert(!networks.remember("failed-network","not-stored-password"));
    Preferences::writesFail()=false;
    KnownNetworks afterFailure;afterFailure.begin();assert(afterFailure.count()==2);
    assert(!afterFailure.matching("failed-network",ssid,password));
    // Explicit forgetting remains durable and does not resurrect a driver profile.
    assert(afterFailure.forget(0));KnownNetworks forgotten;forgotten.begin();
    assert(!forgotten.matching("new-network",ssid,password));
    assert(forgotten.forget(0));KnownNetworks empty;empty.begin();
    strcpy(ssid,"stale");strcpy(password,"stale-password");
    assert(!wifiReconnectCredentials(empty,"",false,"","",ssid,password));
    assert(!ssid[0] && !password[0]);
}
