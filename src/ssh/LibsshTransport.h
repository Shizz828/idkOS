#pragma once
#include "SshData.h"
#include "SshResourceAudit.h"
#include <libssh_esp32.h>
#include <libssh/libssh.h>
#include <lwip/dns.h>
#include <lwip/tcpip.h>
#include <lwip/sockets.h>
#include <fcntl.h>
#include <new>
#include <lwip/tcp.h>
#include <errno.h>
#include <atomic>
namespace spark {
// Two fixed slots cover an active lookup and a canceled lookup whose callback
// has not arrived. No canceled DNS operation can grow a per-retry heap backlog.
struct SshDnsRequest {
    std::atomic<bool> used{false};
    std::atomic<unsigned> references{0};std::atomic<int> status{0};
    char host[97]{};ip_addr_t address{};
    static SshDnsRequest* pool(){static SshDnsRequest slots[2];return slots;}
    static SshDnsRequest* acquire(){
        for(unsigned i=0;i<2;++i){auto& slot=pool()[i];bool available=false;
            if(slot.used.compare_exchange_strong(available,true)){
                slot.references.store(2);slot.status.store(0);slot.address=ip_addr_t{};slot.host[0]=0;return &slot;
            }
        }
        return nullptr;
    }
    static unsigned pending(){unsigned n=0;for(unsigned i=0;i<2;++i)if(pool()[i].used.load())++n;return n;}
    void release(){if(references.fetch_sub(1)==1){sshWipe(host,sizeof(host));used.store(false);}}
    static void found(const char*,const ip_addr_t* address,void* context){auto self=static_cast<SshDnsRequest*>(context);if(address && IP_IS_V4(address)){self->address=*address;self->status.store(1);}else self->status.store(-1);self->release();}
    static void start(void* context){auto self=static_cast<SshDnsRequest*>(context);ip_addr_t address{};err_t result=dns_gethostbyname(self->host,&address,found,self);if(result==ERR_OK)found(nullptr,&address,self);else if(result!=ERR_INPROGRESS)found(nullptr,nullptr,self);}
};
class LibsshTransport {
    bool socketTracked=false;
    SshDnsRequest* dns=nullptr;
    ssh_session session=nullptr;ssh_channel channel=nullptr;
    unsigned opening=0;
    char reason[81]="SSH connection failed";
    SshIo failure(const char* message){sshCopy(reason,message,sizeof(reason));for(char* c=reason;*c;++c)if(uint8_t(*c)<32 || uint8_t(*c)>126)*c=' ';return SshIo::Error;}
    SshIo result(int code){if(code==SSH_OK)return SshIo::Ok;if(code==SSH_AGAIN)return SshIo::Again;return failure(session?ssh_get_error(session):"SSH memory low");}
    void trackSocket(){
        bool open=session && ssh_get_fd(session)!=SSH_INVALID_SOCKET;
        if(open!=socketTracked){if(open)sshResources().sockets.fetch_add(1);else sshResources().sockets.fetch_sub(1);socketTracked=open;}
    }
    SshIo configure(const SshProfile& profile,const char* address){
        session=ssh_new();if(!session)return failure("SSH memory low");
        sshResources().sessions.fetch_add(1);
        unsigned port=profile.port;long timeout=10;bool processConfig=false;int verbosity=SSH_LOG_NOLOG;
        if(ssh_options_set(session,SSH_OPTIONS_PROCESS_CONFIG,&processConfig)!=SSH_OK ||
           ssh_options_set(session,SSH_OPTIONS_HOST,address)!=SSH_OK ||
           ssh_options_set(session,SSH_OPTIONS_PORT,&port)!=SSH_OK ||
           ssh_options_set(session,SSH_OPTIONS_USER,profile.user)!=SSH_OK ||
           ssh_options_set(session,SSH_OPTIONS_TIMEOUT,&timeout)!=SSH_OK ||
           ssh_options_set(session,SSH_OPTIONS_LOG_VERBOSITY,&verbosity)!=SSH_OK)return failure("SSH option setup failed");
        ssh_set_blocking(session,0);return SshIo::Ok;
    }
public:
    static void initialize(){static bool initialized=false;if(!initialized){libssh_begin();initialized=true;}}
    LibsshTransport(){initialize();}
    LibsshTransport(const LibsshTransport&)=delete;
    LibsshTransport& operator=(const LibsshTransport&)=delete;
    ~LibsshTransport(){close();}
    SshIo connect(const SshProfile& profile){
        if(session)return SshIo::Ok;
        ip4_addr_t literal{};
        if(ip4addr_aton(profile.host,&literal))return configure(profile,profile.host);
        if(!dns){
            dns=SshDnsRequest::acquire();if(!dns)return failure("DNS still finishing; retry shortly");
            sshCopy(dns->host,profile.host,sizeof(dns->host));
            if(tcpip_callback(SshDnsRequest::start,dns)!=ERR_OK){dns->release();dns->release();dns=nullptr;return failure("DNS resolver busy");}
            return SshIo::Again;
        }
        int status=dns->status.load();if(!status)return SshIo::Again;
        if(status<0){dns->release();dns=nullptr;return failure("Host unreachable (DNS failed)");}
        char address[16]{};
        ip4addr_ntoa_r(ip_2_ip4(&dns->address),address,sizeof(address));
        dns->release();dns=nullptr;
        // libssh owns the socket from creation through close, including read/
        // write errors. Do not also close a caller-owned fd that lwIP may reuse.
        // Fingerprint trust still uses the original host/port in SshSession.
        return configure(profile,address);
    }
    SshIo handshake(){
        int rc=ssh_connect(session);trackSocket();
        if(rc==SSH_OK){
            int fd=ssh_get_fd(session),enabled=1,idle=30,interval=10,probes=3;
            setsockopt(fd,SOL_SOCKET,SO_KEEPALIVE,&enabled,sizeof(enabled));
            setsockopt(fd,IPPROTO_TCP,TCP_KEEPIDLE,&idle,sizeof(idle));
            setsockopt(fd,IPPROTO_TCP,TCP_KEEPINTVL,&interval,sizeof(interval));
            setsockopt(fd,IPPROTO_TCP,TCP_KEEPCNT,&probes,sizeof(probes));
        }
        return result(rc);
    }
    bool fingerprint(char* output,size_t capacity){
        ssh_key key=nullptr;unsigned char* hash=nullptr;size_t length=0;
        if(ssh_get_server_publickey(session,&key)!=SSH_OK)return false;
        int rc=ssh_get_publickey_hash(key,SSH_PUBLICKEY_HASH_SHA256,&hash,&length);ssh_key_free(key);
        if(rc!=SSH_OK || length!=32 || capacity<51){if(hash)ssh_clean_pubkey_hash(&hash);return false;}
        // OpenSSH's SHA256: plus unpadded base64, 50 visible characters total.
        const char* alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";sshCopy(output,"SHA256:",capacity);unsigned out=7;
        for(unsigned i=0;i<32;i+=3){unsigned n=unsigned(hash[i])<<16;if(i+1<32)n|=unsigned(hash[i+1])<<8;if(i+2<32)n|=hash[i+2];output[out++]=alphabet[n>>18];output[out++]=alphabet[(n>>12)&63];if(i+1<32)output[out++]=alphabet[(n>>6)&63];if(i+2<32)output[out++]=alphabet[n&63];}output[out]=0;ssh_clean_pubkey_hash(&hash);return true;
    }
    SshIo authenticate(const char* secret){int rc=ssh_userauth_password(session,nullptr,secret);if(rc==SSH_AUTH_SUCCESS)return SshIo::Ok;if(rc==SSH_AUTH_AGAIN)return SshIo::Again;if(rc==SSH_AUTH_DENIED || rc==SSH_AUTH_PARTIAL)return SshIo::Denied;return failure("SSH authentication error");}
    SshIo openShell(){
        if(!channel){channel=ssh_channel_new(session);if(!channel)return failure("SSH channel memory low");sshResources().channels.fetch_add(1);}
        int rc=opening==0?ssh_channel_open_session(channel):opening==1?ssh_channel_request_pty_size(channel,"vt100",SshColumns,SshRows):ssh_channel_request_shell(channel);
        if(rc==SSH_OK){if(++opening==3)return SshIo::Ok;return SshIo::Again;}return result(rc);
    }
    int write(const char* data,unsigned size){return ssh_channel_write(channel,data,size);}
    int read(char* output,unsigned size,unsigned stream){return ssh_channel_read_nonblocking(channel,output,size,int(stream));}
    bool eof(){return !ssh_is_connected(session) || ssh_channel_is_eof(channel) || ssh_channel_is_closed(channel);}
    const char* error()const{return reason;}
    void close(){
        if(channel){ssh_channel_send_eof(channel);ssh_channel_close(channel);ssh_channel_free(channel);channel=nullptr;sshResources().channels.fetch_sub(1);}
        if(session){ssh_disconnect(session);ssh_free(session);session=nullptr;sshResources().sessions.fetch_sub(1);}
        if(socketTracked){sshResources().sockets.fetch_sub(1);socketTracked=false;}
        if(dns){dns->release();dns=nullptr;}
    }
};
}
