#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
namespace spark {
constexpr unsigned SshColumns=38,SshRows=7,SshProfileCount=5;
inline void sshWipe(void* data,size_t size) {volatile unsigned char* p=static_cast<volatile unsigned char*>(data);while(size--)*p++=0;}
inline void sshCopy(char* dest,const char* source,size_t capacity) {if(capacity){size_t n=strlen(source);if(n>=capacity)n=capacity-1;memcpy(dest,source,n);dest[n]=0;}}
struct SshProfile {char name[25]{},host[97]{},user[33]{};uint16_t port=22;};
inline bool sshPort(const char* value,uint16_t& port) {
    if(!value[0])return false;
    unsigned n=0;for(const char* p=value;*p;++p){if(*p<'0'||*p>'9')return false;n=n*10+unsigned(*p-'0');if(n>65535)return false;}
    if(!n)return false;
    port=uint16_t(n);return true;
}
inline bool validSshProfile(const SshProfile& p,bool named=false) {
    if(!p.port || !p.host[0] || !p.user[0] || (named && !p.name[0]) || p.name[24] || p.host[96] || p.user[32])return false;
    for(const char* c=p.host;*c;++c)if(!((*c>='a'&&*c<='z')||(*c>='A'&&*c<='Z')||(*c>='0'&&*c<='9')||*c=='.'||*c=='-'||*c=='_'))return false;
    for(const char* c=p.user;*c;++c)if(uint8_t(*c)<33 || uint8_t(*c)>126)return false;
    for(const char* c=p.name;*c;++c)if(uint8_t(*c)<32 || uint8_t(*c)>126)return false;
    return true;
}
inline void normalizeSshHost(char* host) {for(;*host;++host)if(*host>='A'&&*host<='Z')*host+=32;}
enum class SshPhase : uint8_t { Idle,Connecting,Handshake,Verify,Authenticating,Opening,Connected,Disconnected,Failed };
inline bool sshActive(SshPhase phase) {return phase>=SshPhase::Connecting && phase<=SshPhase::Connected;}
struct SshSnapshot {
    SshPhase phase=SshPhase::Idle;uint32_t revision=0;
    SshProfile profile;
    char message[81]{},fingerprint[52]{};
    char rows[SshRows][SshColumns+1]{};
    unsigned cursorX=0,cursorY=0,scroll=0;
};
enum class SshIo { Ok,Again,Error,Denied };
enum class SshTrust { Unknown,Match,Changed,Unavailable };
// Bounded buffer: failed enqueue is atomic; consumed bytes are cleared.
class SshInputBuffer {
    char bytes[512]{};unsigned head=0,count=0;
public:
    unsigned available()const{return unsigned(sizeof(bytes))-count;}
    bool push(const char* data,unsigned size){if(size>sizeof(bytes)-count)return false;for(unsigned i=0;i<size;++i)bytes[(head+count+i)%sizeof(bytes)]=data[i];count+=size;return true;}
    unsigned pop(char* output,unsigned capacity){unsigned n=count<capacity?count:capacity;for(unsigned i=0;i<n;++i){output[i]=bytes[head];bytes[head]=0;head=(head+1)%sizeof(bytes);}count-=n;return n;}
    void clear(){sshWipe(bytes,sizeof(bytes));head=count=0;}
};
}
