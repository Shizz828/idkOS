#include "SshService.h"
#include "../ssh/SshSession.h"
#include "../ssh/SshPersistence.h"
#include "../ssh/LibsshTransport.h"
#include <WiFi.h>
#include <new>
#include <esp_heap_caps.h>
#include "../core/NetworkMemory.h"
namespace spark {
bool SshService::begin(){
    if(worker)return true;
    if(!mutex)mutex=xSemaphoreCreateMutexStatic(&mutexStorage);
    if(!mutex)return false;
    worker=xTaskCreateStaticPinnedToCore(taskEntry,"idkos-ssh",WorkerStackBytes,this,1,workerStack,&workerControl,0);
    return worker!=nullptr;
}
void SshService::logHeap(const char* stage)const{
    const uint32_t caps=MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT;
    Serial.printf("[ssh] attempt=%u %s free=%u largest=%u minimum=%u stack_free_min=%u sessions=%u channels=%u sockets=%u dns=%u https=%u\n",
        attempt.load(),stage,unsigned(heap_caps_get_free_size(caps)),unsigned(heap_caps_get_largest_free_block(caps)),
        unsigned(heap_caps_get_minimum_free_size(caps)),unsigned(uxTaskGetStackHighWaterMark(worker)),
        sshResources().sessions.load(),sshResources().channels.load(),sshResources().sockets.load(),SshDnsRequest::pending(),networkMemory().httpActive());
}
bool SshService::connect(const SshProfile& profile,const char* secret){
    launchError[0]=0;
    if(WiFi.status()!=WL_CONNECTED){sshCopy(launchError,"Wi-Fi unavailable",sizeof(launchError));return false;}
    if(!validSshProfile(profile)){sshCopy(launchError,"Invalid SSH host or username",sizeof(launchError));return false;}
    if(!secret || strlen(secret)>128){sshCopy(launchError,"SSH password exceeds 128 characters",sizeof(launchError));return false;}
    if(!begin()){sshCopy(launchError,"SSH worker initialization failed",sizeof(launchError));return false;}
    if(running.exchange(true)){sshCopy(launchError,"SSH worker busy; retry shortly",sizeof(launchError));return false;}
    cancelled.store(false);decision.store(0);scrollBy.store(0);
    xSemaphoreTake(mutex,portMAX_DELAY);
    requested=profile;sshCopy(password,secret,sizeof(password));outbound.clear();
    uint32_t revision=published.revision+1;published=SshSnapshot{};published.revision=revision;
    published.phase=SshPhase::Connecting;published.profile=profile;
    sshCopy(published.message,"Connecting to host",sizeof(published.message));
    xSemaphoreGive(mutex);
    ++attempt;logHeap("before-start");
    networkMemory().requestSsh();
    xTaskNotifyGive(worker);
    return true;
}

bool SshService::snapshot(SshSnapshot& state){if(!mutex || xSemaphoreTake(mutex,0)!=pdTRUE)return false;state=published;xSemaphoreGive(mutex);return true;}
bool SshService::send(const char* bytes,unsigned size){if(!running.load() || cancelled.load() || !mutex || xSemaphoreTake(mutex,pdMS_TO_TICKS(2))!=pdTRUE)return false;bool ok=published.phase==SshPhase::Connected && outbound.push(bytes,size);xSemaphoreGive(mutex);return ok;}
void SshService::publish(const SshSnapshot& state){xSemaphoreTake(mutex,portMAX_DELAY);if(published.phase==state.phase && !memcmp(&published.profile,&state.profile,sizeof(state.profile)) && !strcmp(published.message,state.message) && !strcmp(published.fingerprint,state.fingerprint) && !memcmp(published.rows,state.rows,sizeof(state.rows)) && published.cursorX==state.cursorX && published.cursorY==state.cursorY && published.scroll==state.scroll){xSemaphoreGive(mutex);return;}uint32_t revision=published.revision+1;published=state;published.revision=revision;xSemaphoreGive(mutex);}
void SshService::taskEntry(void* context){
    auto& self=*static_cast<SshService*>(context);
    LibsshTransport::initialize();
    bool settled=false;uint32_t finished=0;
    for(;;){
        if(ulTaskNotifyTake(pdTRUE,settled?pdMS_TO_TICKS(2000):portMAX_DELAY)){
            self.run(); // Every attempt's objects/destructors finish here.
            self.logHeap(self.published.phase==SshPhase::Failed?"after-failed-cleanup":"after-disconnect-cleanup");
            self.running.store(false);finished=millis();settled=true;
        } else if(settled && !self.running.load() && uint32_t(millis()-finished)>=2000 && !networkMemory().httpActive() && !SshDnsRequest::pending()){
            self.logHeap("settled-idle");settled=false;
        }
    }
}
void SshService::run(){
    SshSession session;SshKnownHosts known;SshInputBuffer input;
    xSemaphoreTake(mutex,portMAX_DELAY);session.begin(requested,password,millis());sshWipe(password,sizeof(password));xSemaphoreGive(mutex);
    bool lease=false;uint32_t waiting=millis();
    sshCopy(session.state.message,"Waiting for network RAM",sizeof(session.state.message));publish(session.state);
    while(!(lease=networkMemory().trySsh())){
        if(cancelled.load()){session.finish("Disconnected");break;}
        if(WiFi.status()!=WL_CONNECTED){session.finish("Wi-Fi unavailable",true);break;}
        if(uint32_t(millis()-waiting)>=60000){session.finish("Network busy; retry SSH",true);break;}
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    if(sshActive(session.state.phase)){
        session.resumeConnecting(millis());logHeap("network-ready");
    }
    if(!sshActive(session.state.phase)){
        networkMemory().cancelSsh();
        xSemaphoreTake(mutex,portMAX_DELAY);outbound.clear();xSemaphoreGive(mutex);
        publish(session.state);return;
    }
    known.begin();LibsshTransport transport;
    uint32_t lastPublish=0;SshPhase previous=SshPhase::Idle;
    while(sshActive(session.state.phase)){
        xSemaphoreTake(mutex,portMAX_DELAY);char bytes[128];unsigned n=outbound.pop(bytes,input.available()<sizeof(bytes)?input.available():sizeof(bytes));if(n)input.push(bytes,n);sshWipe(bytes,sizeof(bytes));xSemaphoreGive(mutex);
        session.tick(millis(),WiFi.status()==WL_CONNECTED,cancelled.load(),decision.exchange(0),transport,known,input,scrollBy.exchange(0));
        if(lease && session.state.phase==SshPhase::Connected){logHeap("connected");networkMemory().releaseSsh();lease=false;}
        uint32_t now=millis();
        if(previous!=session.state.phase){
            unsigned margin=uxTaskGetStackHighWaterMark(nullptr);
            Serial.printf("[ssh] phase=%u free=%u stack_free_min=%u\n",unsigned(session.state.phase),ESP.getFreeHeap(),margin);
            if(margin<2048 && sshActive(session.state.phase))session.finish("SSH stack margin low; stopped",true);
        }
        if(previous!=session.state.phase || uint32_t(now-lastPublish)>=50){publish(session.state);previous=session.state.phase;lastPublish=now;}
        if(sshActive(session.state.phase))vTaskDelay(pdMS_TO_TICKS(10));
    }
    transport.close();input.clear();
    if(lease)networkMemory().releaseSsh();else networkMemory().cancelSsh();
    xSemaphoreTake(mutex,portMAX_DELAY);outbound.clear();xSemaphoreGive(mutex);
    publish(session.state);
}
}
