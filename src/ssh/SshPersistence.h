#pragma once
#include <Preferences.h>
#include "SshData.h"
namespace spark {
inline uint32_t sshChecksum(const void* data,size_t size){uint32_t hash=2166136261u;auto bytes=static_cast<const uint8_t*>(data);for(size_t i=0;i<size;++i)hash=(hash^bytes[i])*16777619u;return hash;}
class SshProfiles {
    struct Blob {uint32_t version=1;SshProfile entries[SshProfileCount];uint32_t checksum=0;} blob;
    bool save(){blob.checksum=sshChecksum(&blob,offsetof(Blob,checksum));Preferences p;if(!p.begin("spark-ssh",false))return false;bool ok=p.putBytes("profiles",&blob,sizeof(blob))==sizeof(blob);p.end();return ok;}
public:
    bool begin(){Preferences p;if(!p.begin("spark-ssh",true))return true;size_t n=p.getBytesLength("profiles");bool ok=!n || (n==sizeof(blob) && p.getBytes("profiles",&blob,sizeof(blob))==sizeof(blob) && blob.version==1 && blob.checksum==sshChecksum(&blob,offsetof(Blob,checksum)));p.end();if(ok && n)for(const auto& entry:blob.entries)if(entry.host[0] && !validSshProfile(entry,true))ok=false;if(!ok)blob=Blob{};return ok;}
    unsigned count()const{unsigned n=0;for(const auto& entry:blob.entries)if(entry.host[0])++n;return n;}
    const SshProfile& at(unsigned i)const{return blob.entries[i<SshProfileCount?i:0];}
    bool put(SshProfile profile){normalizeSshHost(profile.host);if(!validSshProfile(profile,true))return false;unsigned index=SshProfileCount;for(unsigned i=0;i<SshProfileCount;++i)if(!strcmp(profile.name,blob.entries[i].name)){index=i;break;}if(index==SshProfileCount)for(unsigned i=0;i<SshProfileCount;++i)if(!blob.entries[i].host[0]){index=i;break;}if(index==SshProfileCount)return false;Blob old=blob;blob.entries[index]=profile;if(save())return true;blob=old;return false;}
    bool erase(unsigned index){if(index>=count())return false;Blob old=blob;for(unsigned i=index;i+1<SshProfileCount;++i)blob.entries[i]=blob.entries[i+1];blob.entries[SshProfileCount-1]=SshProfile{};if(save())return true;blob=old;return false;}
};
class SshKnownHosts {
    struct Host {char host[97]{},fingerprint[52]{};uint16_t port=0;};
    struct Blob {uint32_t version=1;Host entries[8];uint32_t checksum=0;} blob;
    bool valid=true;
public:
    void begin(){Preferences p;if(!p.begin("spark-ssh-keys",false)){valid=false;return;}size_t n=p.getBytesLength("hosts");valid=!n || (n==sizeof(blob) && p.getBytes("hosts",&blob,sizeof(blob))==sizeof(blob) && blob.version==1 && blob.checksum==sshChecksum(&blob,offsetof(Blob,checksum)));p.end();if(valid)for(const auto& h:blob.entries)if(h.host[0] && (h.host[96] || h.fingerprint[51] || strlen(h.fingerprint)!=50 || strncmp(h.fingerprint,"SHA256:",7) || !h.port))valid=false;}
    SshTrust check(const SshProfile& profile,const char* fingerprint)const{if(!valid)return SshTrust::Unavailable;for(const auto& h:blob.entries)if(h.port==profile.port && !strcmp(h.host,profile.host))return !strcmp(h.fingerprint,fingerprint)?SshTrust::Match:SshTrust::Changed;return SshTrust::Unknown;}
    bool remember(const SshProfile& profile,const char* fingerprint){if(!valid || strlen(fingerprint)!=50 || strncmp(fingerprint,"SHA256:",7))return false;if(check(profile,fingerprint)==SshTrust::Match)return true;if(check(profile,fingerprint)!=SshTrust::Unknown)return false;for(auto& h:blob.entries)if(!h.host[0]){Blob old=blob;sshCopy(h.host,profile.host,sizeof(h.host));sshCopy(h.fingerprint,fingerprint,sizeof(h.fingerprint));h.port=profile.port;blob.checksum=sshChecksum(&blob,offsetof(Blob,checksum));Preferences p;bool ok=false;if(p.begin("spark-ssh-keys",false)){ok=p.putBytes("hosts",&blob,sizeof(blob))==sizeof(blob);p.end();}if(!ok)blob=old;return ok;}return false;}
};
}
