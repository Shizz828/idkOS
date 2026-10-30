#pragma once
#include "../core/Input.h"
#include "../ssh/SshPersistence.h"
#include "../ui/Screen.h"
namespace spark {
class SshApp {
    enum Page {Profiles,Form,Session} page=Form;
    SshProfiles profiles;
    SshSnapshot state;
    SshProfile profile;
    char port[6]="22",password[129]{},message[81]{};
    unsigned field=1,selected=0;uint32_t revision=0;
    bool loaded=false,confirmDisconnect=false,confirmDelete=false;
    void say(const char* value){sshCopy(message,value,sizeof(message));}
    void edit(const SshProfile& value){profile=value;snprintf(port,sizeof(port),"%u",unsigned(profile.port));sshWipe(password,sizeof(password));field=4;page=Form;message[0]=0;}
    bool readProfile(){
        if(!sshPort(port,profile.port)){say("Port must be 1-65535");return false;}
        normalizeSshHost(profile.host);
        if(!validSshProfile(profile)){
            if(!profile.host[0]){say("Host/IP is required");field=1;}
            else if(!profile.user[0]){say("Username is required");field=3;}
            else say("Invalid host/username; check spaces");
#if defined(ARDUINO)
            Serial.printf("[ssh-form] validation rejected host_len=%u user_len=%u name_len=%u port=%u\n",
                unsigned(strlen(profile.host)),unsigned(strlen(profile.user)),unsigned(strlen(profile.name)),unsigned(profile.port));
#endif
            return false;
        }
        return true;
    }
    void shortText(int x,int y,const char* value,unsigned width=38,uint16_t color=theme::Content)const{char line[39]{};snprintf(line,sizeof(line),"%.*s",int(width),value);text(x,y,line,1,color);}
public:
    ~SshApp(){sshWipe(password,sizeof(password));}
    template<typename Service> void open(Service& service){
        if(!loaded){loaded=true;if(!profiles.begin())say("Saved profiles unreadable");}
        SshSnapshot current;if(service.snapshot(current)){state=current;revision=current.revision;}
        confirmDelete=confirmDisconnect=false;
        if(sshActive(state.phase) || service.busy())page=Session;
        else page=profiles.count()?Profiles:Form;
    }
    void leave(){sshWipe(password,sizeof(password));confirmDelete=confirmDisconnect=false;}
    template<typename Service> bool poll(Service& service){
        SshSnapshot next;if(!service.snapshot(next) || next.revision==revision)return false;
        revision=next.revision;bool wasActive=sshActive(state.phase);state=next;
        if(sshActive(next.phase))page=Session;
        else if(wasActive && (next.phase==SshPhase::Failed || next.phase==SshPhase::Disconnected)){edit(next.profile);say(next.message);confirmDisconnect=false;}
        return true;
    }
    template<typename Service> bool back(Service& service){
        if(confirmDisconnect || confirmDelete){confirmDisconnect=confirmDelete=false;return true;}
        if(sshActive(state.phase) || service.busy()){
            if(state.phase==SshPhase::Connected)confirmDisconnect=true;
            else {service.disconnect();say("Canceling connection");}
            return true;
        }
        sshWipe(password,sizeof(password));
        if(page==Form && profiles.count()){page=Profiles;return true;}
        return false;
    }
    template<typename Service> void input(const InputEvent& e,Service& service,bool online){
#if defined(ARDUINO)
        if(e.enter)Serial.printf("[ssh-form] enter page=%u field=%u online=%u host_len=%u user_len=%u password_len=%u phase=%u\n",
            unsigned(page),field,online,unsigned(strlen(profile.host)),unsigned(strlen(profile.user)),unsigned(strlen(password)),unsigned(state.phase));
#endif
        if(confirmDisconnect){
            if(e.text[0]=='y'||e.text[0]=='Y'){service.disconnect();confirmDisconnect=false;say("Disconnecting");}
            else if(e.enter || e.text[0])confirmDisconnect=false;
            return;
        }
        if(confirmDelete){if(e.text[0]=='y'||e.text[0]=='Y'){say(profiles.erase(selected)?"Profile removed":"Profile removal failed");if(selected>=profiles.count())selected=0;}if(e.enter||e.text[0])confirmDelete=false;return;}
        if(page==Session){
            if(state.phase==SshPhase::Verify){char c=e.text[0];if(c=='y'||c=='Y')service.verify(1);else if(c=='o'||c=='O')service.verify(2);else if(e.enter||c=='n'||c=='N')service.verify(-1);return;}
            if(state.phase!=SshPhase::Connected)return;
            if(e.shift && (e.up||e.down)){service.scroll(e.up?3:-3);return;}
            char bytes[64]{};unsigned n=0;
            if(e.control)bytes[n++]=e.control;
            else if(e.tab)bytes[n++]='\t';
            else {for(const char* c=e.text;*c && n<56;++c)bytes[n++]=*c;}
            if(e.erase)bytes[n++]=127;
            if(e.enter)bytes[n++]='\r';
            const char* arrow=e.up?"\033[A":e.down?"\033[B":e.right?"\033[C":e.left?"\033[D":nullptr;
            if(arrow){memcpy(bytes+n,arrow,3);n+=3;}
            if(n){service.scroll(-48);if(!service.send(bytes,n))say("SSH input busy; retry key");else message[0]=0;}sshWipe(bytes,sizeof(bytes));return;
        }
        if(page==Profiles){
            if(e.up && selected)--selected;
            if(e.down && selected+1<profiles.count())++selected;
            if(e.text[0]=='n'||e.text[0]=='N'){profile=SshProfile{};sshCopy(port,"22",sizeof(port));field=0;sshWipe(password,sizeof(password));message[0]=0;page=Form;}
            if(e.control==4 && profiles.count())confirmDelete=true;
            if(e.enter && profiles.count())edit(profiles.at(selected));
            return;
        }
        if(e.save){if(readProfile()){if(!profile.name[0])say("Enter a profile name to save");else say(profiles.put(profile)?"Profile saved; password not stored":"Save failed or 5 profiles full");}return;}
        if(e.up && field)--field;
        if(e.down && field<4)++field;
        if(e.tab){field=(field+1)%5;return;}
        char* target=field==0?profile.name:field==1?profile.host:field==2?port:field==3?profile.user:password;
        size_t capacity=field==0?sizeof(profile.name):field==1?sizeof(profile.host):field==2?sizeof(port):field==3?sizeof(profile.user):sizeof(password);
        size_t length=strlen(target);if(e.erase && length){target[--length]=0;}
        for(const char* c=e.text;*c && length+1<capacity;++c){target[length++]=*c;target[length]=0;}
        if(!e.enter)return;
        if(field<4){++field;return;}
        if(!readProfile())return;
        if(!online){say("Wi-Fi unavailable");return;}
#if defined(ARDUINO)
        Serial.printf("[ssh-form] connect callback host_len=%u user_len=%u password_len=%u port=%u\n",
            unsigned(strlen(profile.host)),unsigned(strlen(profile.user)),unsigned(strlen(password)),unsigned(profile.port));
#endif
        bool accepted=service.connect(profile,password);
#if defined(ARDUINO)
        Serial.printf("[ssh-form] connect accepted=%u reason=%s\n",accepted,accepted?"queued":service.startError());
#endif
        if(accepted) {page=Session;state.phase=SshPhase::Connecting;state.profile=profile;sshCopy(state.message,"Connecting to host",sizeof(state.message));confirmDisconnect=false;message[0]=0;}
        else {
            say(service.startError());
            // A second attempt must return to an existing connection rather
            // than leave the user stuck on a generic "busy" login error.
            SshSnapshot current;
            if(service.snapshot(current) && sshActive(current.phase)){
                state=current;revision=current.revision;page=Session;confirmDisconnect=false;
            }
        }
        sshWipe(password,sizeof(password));
    }
    void draw()const{
        frame("SSH");
        if(confirmDisconnect){text(6,44,"Disconnect SSH session? [y/N]",1,theme::Attention);text(6,70,"Y disconnects; Enter/Esc keeps it");return;}
        if(confirmDelete){text(6,40,"Remove saved SSH profile? [y/N]",1,theme::Attention);shortText(6,60,profiles.at(selected).name);return;}
        if(page==Profiles){
            text(6,31,"SAVED SSH HOSTS",1,theme::Primary);unsigned start=selected>=4?selected-3:0;
            for(unsigned i=start;i<profiles.count() && i<start+4;++i){text(6,47+int(i-start)*13,i==selected?">":" ",1,theme::Primary);shortText(18,47+int(i-start)*13,profiles.at(i).name,35);}
            shortText(6,103,message,38,theme::Attention);text(6,115,"Enter choose  N new  Ctrl+D remove");text(6,126,"Esc back  Fn+Esc sleep");return;
        }
        if(page==Form){
            const char* labels[]={"Name:","Host/IP:","Port:","Username:","Password:"};
            const char* values[]={profile.name,profile.host,port,profile.user,password};
            for(unsigned i=0;i<5;++i){int y=32+int(i)*14;text(6,y,i==field?">":" ",1,theme::Primary);text(18,y,labels[i],1,i==field?theme::Primary:theme::Content);
                char value[25]{};size_t n=strlen(values[i]);if(i==4){memset(value,'*',n>24?24:n);}else sshCopy(value,values[i]+(i==field && n>24?n-24:0),sizeof(value));text(90,y,value,1,i==field?theme::Primary:theme::Content);}
            shortText(6,103,message,38,theme::Attention);text(6,115,"Enter next/connect  Ctrl+S save");text(6,126,"Fn up/down field  Tab next  Esc back");return;
        }
        if(state.phase==SshPhase::Verify){
            text(6,31,"VERIFY SERVER KEY",1,theme::Attention);
            char endpoint[106];snprintf(endpoint,sizeof(endpoint),"%s:%u",state.profile.host,unsigned(state.profile.port));
            shortText(6,43,endpoint);if(strlen(endpoint)>38)shortText(6,53,endpoint+38);if(strlen(endpoint)>76)shortText(6,63,endpoint+76);
            shortText(6,76,state.fingerprint,38,theme::Primary);shortText(6,87,state.fingerprint+38,38,theme::Primary);
            shortText(6,102,state.message,38,theme::Attention);text(6,115,"Y accept/save  O once  N reject");text(6,126,"Compare with server before accepting");return;
        }
        char identity[39];snprintf(identity,sizeof(identity),"%.12s@%.24s",state.profile.user,state.profile.host);
        if(state.phase==SshPhase::Connected){
            text(6,29,"SSH: Connected",1,theme::Success);text(6,40,identity,1,theme::Primary);
            for(unsigned row=0;row<SshRows;++row)text(6,51+int(row)*10,state.rows[row]);
            if(!state.scroll && state.cursorY<SshRows && state.cursorX<SshColumns)screen().drawFastHLine(6+int(state.cursorX)*6,59+int(state.cursorY)*10,5,theme::Primary);
            text(6,126,message[0]?message:state.scroll?"SCROLLBACK: Shift+Fn up/down":"Esc disconnect  Shift+Fn arrows scroll",1,message[0]?theme::Error:theme::Primary);return;
        }
        text(6,38,identity,1,theme::Primary);shortText(6,61,state.message,38,theme::Attention);if(strlen(state.message)>38)shortText(6,73,state.message+38);
        text(6,103,"Esc cancels; other apps stay usable");text(6,126,"Fn+Esc sleep  G0 home");
    }
};
}
