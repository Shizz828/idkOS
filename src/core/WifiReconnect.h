#pragma once
#include <string.h>
namespace spark {
// Unsaved credentials from Settings must survive radio restarts and retries.
// Saved profiles remain the single durable store; this selects runtime input only.
template<class Networks>
bool wifiReconnectCredentials(Networks& networks,const char* preferredSsid,
                              bool pending,const char* pendingSsid,const char* pendingPassword,
                              char* ssid,char* password) {
    ssid[0]=0;password[0]=0;
    if(pending && pendingSsid[0]) {
        strlcpy(ssid,pendingSsid,33);strlcpy(password,pendingPassword,65);return true;
    }
    return networks.matching(preferredSsid,ssid,password) || networks.candidate(ssid,password);
}
}
