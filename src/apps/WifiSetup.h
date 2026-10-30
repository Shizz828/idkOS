#pragma once
#include "../core/Input.h"
#include "../services/WeatherData.h"
#include "../ui/Screen.h"
#include <ctype.h>
namespace spark {
class WifiSetup {
    char ssid[33]{}, password[65]{};
    unsigned field=0,selected=0;
    bool choosing=false,scanPending=false;
    uint32_t scanBaseline=0;
    WeatherSnapshot::NearbyNetwork nearby[12];
    unsigned nearbyCount=0;
    template<typename Network> void scan(Network& service,uint32_t revision) {
        scanBaseline=revision;
        scanPending=service.networkAction(4);
        message=scanPending?"Scanning; please wait":"Wi-Fi worker busy; R retries";
    }
    uint32_t pendingAttempt=0;
    const char* message="Enter SSID, then password";
public:
    bool editing=false;
    // The nearby-network list is a menu; the SSID/password form is text entry.
    bool choosingNetwork() const { return editing && choosing; }
    void open() { choosing=false; scanPending=false; editing=true; field=0; pendingAttempt=0; message="Enter SSID, then password"; }
    template<typename Network> void open(Network& service,const WeatherSnapshot& state) {
        open();choosing=true;selected=0;nearbyCount=0;scan(service,state.scanRevision);
    }
    void cancel() { editing=false; choosing=false; scanPending=false; pendingAttempt=0; memset(password,0,sizeof(password)); }
    void sync(const WeatherSnapshot& state) {
        if(editing && choosing && scanPending && !state.scanning && state.scanRevision!=scanBaseline) {
            scanPending=false;scanBaseline=state.scanRevision;nearbyCount=state.nearbyCount;selected=0;
            memcpy(nearby,state.nearby,sizeof(nearby));
            message=state.scanFailed?"Scan failed; R retries":nearbyCount?"Select network; Enter for password":"No networks; R retry / M manual";
        }
        if(!editing || !pendingAttempt || state.wifi.attempt!=pendingAttempt) return;
        if(state.wifi.phase==WifiPhase::Connected) cancel();
        else if(state.wifi.phase!=WifiPhase::Connecting) {
            pendingAttempt=0;
            message=wifiMessage(state.wifi.phase);
        }
    }
    template<typename Network>
    void input(const InputEvent& e, Network& service) {
        if(choosing) {
            if(e.text[0]=='m' || e.text[0]=='M') {
                choosing=false;scanPending=false;field=0;ssid[0]=0;
                memset(password,0,sizeof(password));message="Enter SSID, then password";
            } else {
                if((e.text[0]=='r' || e.text[0]=='R') && !scanPending) {
                    scan(service,scanBaseline);
                }
                if(e.up && selected)--selected;
                if(e.down && selected+1<nearbyCount)++selected;
                if(e.enter && !scanPending && nearbyCount) {
                    strlcpy(ssid,nearby[selected].ssid,sizeof(ssid));
                    memset(password,0,sizeof(password));field=1;choosing=false;
                    message="Blank password: open network";
                }
            }
            return;
        }
        if(pendingAttempt) return;
        if(e.up) { field=0; message="Edit SSID; Enter advances"; }
        if(e.down) { field=1; message="Edit password; Enter connects"; }
        char* buffer=field ? password : ssid;
        size_t capacity=field ? sizeof(password) : sizeof(ssid);
        size_t length=strlen(buffer);
        if(e.erase && length) { buffer[--length]=0; message="Enter: next / connect"; }
        for(const char* p=e.text;*p;++p) {
            if(length+1<capacity) { buffer[length++]=*p; buffer[length]=0; }
            else message="Field full";
        }
        if(e.enter) {
            if(!field) {
                if(!ssid[0]) message="SSID is required";
                else { field=1; message="Blank password: open network"; }
            } else {
                bool valid=strlen(password)==0 || (strlen(password)>=8 && strlen(password)<=63);
                if(strlen(password)==64) {
                    valid=true;
                    for(const char* p=password;*p;++p) if(!isxdigit(static_cast<unsigned char>(*p))) valid=false;
                }
                if(!valid) message="Password: 8-63 chars or 64 hex";
                else {
                    pendingAttempt=service.connect(ssid,password);
                    message=pendingAttempt ? "Request queued; please wait" : "Wi-Fi setup busy; retry";
                }
            }
        }
    }
    void draw(const WeatherSnapshot& state,const char* title="WI-FI SETUP") const {
        frame(title);
        if(choosing) {
            text(6,32,message,1,theme::Attention);
            unsigned start=selected>=5?selected-4:0;
            for(unsigned i=start;i<nearbyCount && i<start+5;++i) {
                char name[30];snprintf(name,sizeof(name),"%c %.27s",i==selected?'>':' ',nearby[i].ssid);
                text(6,46+int(i-start)*13,name,1,theme::Primary);
                char signal[12];snprintf(signal,sizeof(signal),"%d",nearby[i].rssi);
                text(210,46+int(i-start)*13,signal);
            }
            text(6,112,"Enter select  R scan  M manual");
            text(6,125,"Up/down  Esc back");return;
        }
        text(6,32,field==0?"> SSID":"  SSID",1,theme::Primary);
        text(6,44,ssid);
        text(6,60,field==1?"> PASSWORD":"  PASSWORD",1,theme::Primary);
        char mask[33]{}; size_t length=strlen(password);
        memset(mask,'*',length>32?32:length); text(6,72,mask);
        text(6,87,message);
        char status[38]{}; strlcpy(status,wifiMessage(state.wifi.phase),sizeof(status)); text(6,101,status);
        text(6,123,"Fn up/down field  Esc cancel");
    }
};
}
