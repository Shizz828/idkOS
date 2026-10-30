#pragma once
#include "SshTerminal.h"
namespace spark {
// All transport calls belong to the worker. Only snapshots cross into the UI.
class SshSession {
    char password[129]{};
    uint32_t stageStarted=0,writeStarted=0;
    char pending[128]{};unsigned pendingSize=0,pendingSent=0;
    void phase(SshPhase next,const char* message,uint32_t now){state.phase=next;sshCopy(state.message,message,sizeof(state.message));stageStarted=now;}
    const char* timeoutMessage()const{
        switch(state.phase){
        case SshPhase::Connecting:return "SSH address setup timed out";
        case SshPhase::Handshake:return "SSH negotiation timed out";
        case SshPhase::Verify:return "Host verification timed out";
        case SshPhase::Authenticating:return "SSH authentication timed out";
        case SshPhase::Opening:return "SSH shell setup timed out";
        default:return "Connection timed out";
        }
    }
public:
    SshSnapshot state;
    SshTerminal terminal;
    ~SshSession(){sshWipe(password,sizeof(password));sshWipe(pending,sizeof(pending));}
    void begin(SshProfile profile,const char* secret,uint32_t now){normalizeSshHost(profile.host);state.profile=profile;sshCopy(password,secret,sizeof(password));phase(SshPhase::Connecting,"Connecting to host",now);}
    void resumeConnecting(uint32_t now){phase(SshPhase::Connecting,"Connecting to host",now);}
    void finish(const char* reason,bool error=false){sshWipe(password,sizeof(password));sshWipe(pending,sizeof(pending));pendingSize=pendingSent=0;state.phase=error?SshPhase::Failed:SshPhase::Disconnected;sshCopy(state.message,reason,sizeof(state.message));}
    template<typename Transport,typename Trust> void tick(uint32_t now,bool wifi,bool cancel,int decision,Transport& transport,Trust& trust,SshInputBuffer& input,int scroll=0){
        if(!sshActive(state.phase))return;
        if(cancel){finish("Disconnected");return;}if(!wifi){finish("Wi-Fi unavailable",true);return;}
        uint32_t timeout=state.phase==SshPhase::Verify?120000:15000;
        if(state.phase!=SshPhase::Connected && uint32_t(now-stageStarted)>=timeout){finish(timeoutMessage(),true);return;}
        SshIo result=SshIo::Again;
        switch(state.phase){
        case SshPhase::Connecting:
            result=transport.connect(state.profile);
            if(result==SshIo::Ok)phase(SshPhase::Handshake,"Negotiating SSH",now);
            break;
        case SshPhase::Handshake:
            result=transport.handshake();
            if(result==SshIo::Ok){
                if(!transport.fingerprint(state.fingerprint,sizeof(state.fingerprint))){finish("Cannot verify server key",true);return;}
                SshTrust known=trust.check(state.profile,state.fingerprint);
                if(known==SshTrust::Changed){finish("Host key changed; connection refused",true);return;}
                if(known==SshTrust::Unavailable){finish("Saved host keys unreadable",true);return;}
                phase(known==SshTrust::Match?SshPhase::Authenticating:SshPhase::Verify,known==SshTrust::Match?"Authenticating":"Compare server fingerprint",now);
            }
            break;
        case SshPhase::Verify:
            if(decision<0){finish("Host key rejected");return;}
            if(decision==1 || decision==2){
                if(decision==1 && !trust.remember(state.profile,state.fingerprint)){sshCopy(state.message,"Key save failed/full; O once or N",sizeof(state.message));return;}
                phase(SshPhase::Authenticating,"Authenticating",now);
            }
            break;
        case SshPhase::Authenticating:
            result=transport.authenticate(password);
            if(result!=SshIo::Again)sshWipe(password,sizeof(password));
            if(result==SshIo::Denied){finish("Authentication failed",true);return;}
            if(result==SshIo::Ok)phase(SshPhase::Opening,"Opening remote shell",now);
            break;
        case SshPhase::Opening:
            result=transport.openShell();
            if(result==SshIo::Ok)phase(SshPhase::Connected,"SSH: Connected",now);
            break;
        case SshPhase::Connected: {
            if(scroll)terminal.scroll(scroll);
            if(!pendingSize){pendingSize=input.pop(pending,sizeof(pending));pendingSent=0;writeStarted=now;}
            if(pendingSize){
                int n=transport.write(pending+pendingSent,pendingSize-pendingSent);
                if(n<0 && n!=-2){finish("SSH write failed",true);return;}
                if(n>0){sshWipe(pending+pendingSent,size_t(n));pendingSent+=unsigned(n);writeStarted=now;}
                if(pendingSent==pendingSize)pendingSize=pendingSent=0;
                else if(uint32_t(now-writeStarted)>=10000){finish("SSH write timed out",true);return;}
            }
            char bytes[256];
            // A noisy server gets a bounded work budget so UI/service tasks run.
            bool received=false;
            for(unsigned stream=0;stream<2;++stream)for(unsigned batch=0;batch<4;++batch){
                int n=transport.read(bytes,sizeof(bytes),stream);
                if(n>0){terminal.feed(bytes,size_t(n));sshWipe(bytes,size_t(n));received=true;}
                else {if(n<0 && n!=-2 && n!=-127){finish("SSH connection lost",true);return;}break;}
            }
            terminal.snapshot(state);
            // Drain final output before returning to the connection screen.
            if(!received && transport.eof()){finish("Remote shell closed");return;}
            break;
        }
        default:break;
        }
        if(result==SshIo::Error)finish(transport.error(),true);
    }
};
}
