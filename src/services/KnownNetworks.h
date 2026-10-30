#pragma once
#include <Preferences.h>
#include <string.h>
#include <stddef.h>
namespace spark {
class KnownNetworks {
    struct Network { char ssid[33]{},password[65]{}; };
    struct Record { uint32_t version=2; Network entries[5]; uint32_t checksum=0; } record;
    unsigned next=0;
    uint32_t checksum() const {
        uint32_t h=2166136261u; auto p=reinterpret_cast<const uint8_t*>(&record);
        for(size_t i=0;i<offsetof(Record,checksum);++i) h=(h^p[i])*16777619u;
        return h;
    }
public:
    unsigned count() const { unsigned n=0; for(const auto& p:record.entries) if(p.ssid[0]) ++n; return n; }
    void begin() {
        Preferences prefs;
        if(prefs.begin("spark-networks",true)) {
            if(prefs.getBytesLength("profiles")==sizeof(record)) prefs.getBytes("profiles",&record,sizeof(record));
            else {
                struct Legacy { uint32_t version=1; Network entries[3]; uint32_t checksum=0; } old;
                if(prefs.getBytesLength("profiles")==sizeof(old) && prefs.getBytes("profiles",&old,sizeof(old))==sizeof(old)) {
                    uint32_t hash=2166136261u;auto bytes=reinterpret_cast<const uint8_t*>(&old);
                    for(size_t i=0;i<offsetof(Legacy,checksum);++i) hash=(hash^bytes[i])*16777619u;
                    bool valid=old.version==1 && old.checksum==hash;
                    for(const auto& entry:old.entries) if(entry.ssid[32] || entry.password[64])valid=false;
                    if(valid){for(unsigned i=0;i<3;++i)record.entries[i]=old.entries[i];record.checksum=checksum();}
                }
            }
            prefs.end();
        }
        bool valid=record.version==2 && record.checksum==checksum();
        for(const auto& p:record.entries) if(p.ssid[32] || p.password[64]) valid=false;
        if(!valid) record=Record{};
    }
    bool remember(const char* ssid,const char* password) {
        unsigned index=4;
        for(unsigned i=0;i<5;++i) if(!strcmp(record.entries[i].ssid,ssid) || !record.entries[i].ssid[0]) { index=i; break; }
        if(!strcmp(record.entries[index].ssid,ssid) && !strcmp(record.entries[index].password,password)) return true;
        Record previous=record;
        // Keep the most recently accepted network first; evict the oldest if full.
        for(unsigned i=index;i>0;--i) record.entries[i]=record.entries[i-1];
        strlcpy(record.entries[0].ssid,ssid,sizeof(record.entries[0].ssid));
        strlcpy(record.entries[0].password,password,sizeof(record.entries[0].password));
        record.checksum=checksum(); Preferences prefs;
        bool ok=false;
        if(prefs.begin("spark-networks",false)) { ok=prefs.putBytes("profiles",&record,sizeof(record))==sizeof(record); prefs.end(); }
        if(!ok) record=previous;
        return ok;
    }
    void names(char output[5][33]) const { for(unsigned i=0;i<5;++i) strlcpy(output[i],record.entries[i].ssid,33); }
    bool selected(unsigned index,char* ssid,char* password) const {
        if(index>=5 || !record.entries[index].ssid[0]) return false;
        strlcpy(ssid,record.entries[index].ssid,33);strlcpy(password,record.entries[index].password,65);return true;
    }
    bool matching(const char* name,char* ssid,char* password) const {
        for(unsigned i=0;i<5;++i) if(!strcmp(record.entries[i].ssid,name) && name[0]) return selected(i,ssid,password);
        return false;
    }
    bool forget(unsigned index) {
        if(index>=5 || !record.entries[index].ssid[0]) return false;
        Record previous=record;
        for(unsigned i=index;i<4;++i) record.entries[i]=record.entries[i+1];
        record.entries[4]=Network{};record.checksum=checksum();Preferences prefs;
        bool ok=false;if(prefs.begin("spark-networks",false)) {ok=prefs.putBytes("profiles",&record,sizeof(record))==sizeof(record);prefs.end();}
        if(!ok) record=previous;
        next=0;return ok;
    }
    bool candidate(char* ssid,char* password) {
        for(unsigned i=0;i<5;++i) {
            const auto& p=record.entries[next++%5];
            if(!p.ssid[0]) continue;
            strlcpy(ssid,p.ssid,33); strlcpy(password,p.password,65); return true;
        }
        return false;
    }
};
}
