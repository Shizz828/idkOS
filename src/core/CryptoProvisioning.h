#pragma once
#include <Arduino.h>
#include "../services/CryptoService.h"
namespace spark {
// Bounded USB configuration protocol. Credentials are never echoed.
class CryptoProvisioning {
    char line[160]{};
    unsigned length=0;
    bool overflow=false;
public:
    void poll(CryptoService& service) {
        for(unsigned n=0;n<64 && Serial.available();++n) {
            char c=char(Serial.read());
            if(c=='\r') continue;
            if(c=='\n') {
                const char* prefix="IDKOS_CRYPTO_KEY ";
                if(!overflow && strncmp(line,prefix,strlen(prefix))==0) {
                    if(!service.configureKey(line+strlen(prefix))) Serial.println("IDKOS_CRYPTO_KEY_REJECTED");
                }
                memset(line,0,sizeof(line)); length=0; overflow=false;
            } else if(length+1<sizeof(line) && !overflow) { line[length++]=c; line[length]=0; }
            else overflow=true;
        }
    }
};
}
