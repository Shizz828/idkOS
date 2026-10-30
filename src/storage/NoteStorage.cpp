#include "NoteStorage.h"
#include "NoteTransaction.h"
#include "NoteDirectory.h"
#include "NoteReminderStorage.h"
#include "../terminal/SdShell.h"
#include "../terminal/SystemShell.h"
#include "../core/ReminderEngine.h"
#include <SD.h>
#include <SPI.h>
#include <esp_system.h>
namespace spark {
static bool validId(const char* id) {
    if(strlen(id)!=16) return false;
    for(unsigned i=0;i<16;++i) if(!isxdigit(static_cast<unsigned char>(id[i]))) return false;
    return true;
}
static void pathFor(char* output,size_t capacity,const char* id,const char* extension="txt") {
    snprintf(output,capacity,"/idkos/notes/%s.%s",id,extension);
}
bool NoteStorage::begin() {
    mutex=xSemaphoreCreateMutex(); queue=xQueueCreate(1,1);
    if(mutex && queue && xTaskCreatePinnedToCore(taskEntry,"idkos-notes",16384,this,1,nullptr,0)==pdPASS) return true;
    if(mutex) vSemaphoreDelete(mutex);
    if(queue) vQueueDelete(queue);
    mutex=nullptr; queue=nullptr; return false;
}
bool NoteStorage::submit(NoteOperation operation,const char* id,const char* data) {
    if(!mutex || !queue || strlen(data)>NoteLimit || (*id && !validId(id))) return false;
    if(xSemaphoreTake(mutex,0)!=pdTRUE) return false;
    if(inFlight) { xSemaphoreGive(mutex); return false; }
    request.operation=operation; strlcpy(request.id,id,sizeof(request.id)); strlcpy(request.data,data,sizeof(request.data));
    inFlight=true; published.busy=true; published.started=millis(); ++published.revision;
    uint8_t signal=1; bool ok=xQueueSend(queue,&signal,0)==pdTRUE;
    if(!ok) { inFlight=false; published.busy=false; }
    xSemaphoreGive(mutex); return ok;
}
bool NoteStorage::snapshot(NoteStorageState& output) {
    if(!mutex || xSemaphoreTake(mutex,0)!=pdTRUE) return false;
    output=published; xSemaphoreGive(mutex); return true;
}
bool NoteStorage::submitTerminal(const char* command,const char* cwd,bool confirmed,bool system) {
    if(!mutex || !queue || strlen(command)>TerminalCommandLimit || strlen(cwd)>=sizeof(terminalCwd)) return false;
    if(xSemaphoreTake(mutex,0)!=pdTRUE) return false;
    if(inFlight) { xSemaphoreGive(mutex); return false; }
    strlcpy(terminalCommand,command,sizeof(terminalCommand)); strlcpy(terminalCwd,cwd,sizeof(terminalCwd)); terminalConfirmed=confirmed;terminalSystem=system;
    inFlight=true; terminalPublished.busy=true; terminalPublished.started=millis(); ++terminalPublished.revision;
    uint8_t signal=2; bool ok=xQueueSend(queue,&signal,0)==pdTRUE;
    if(!ok) {inFlight=false;terminalPublished.busy=false;}
    xSemaphoreGive(mutex); return ok;
}
bool NoteStorage::terminalSnapshot(TerminalResult& output) {
    if(!mutex || xSemaphoreTake(mutex,0)!=pdTRUE) return false;
    output=terminalPublished; xSemaphoreGive(mutex); return true;
}
bool NoteStorage::requestStats() {
    if(!mutex || !queue || xSemaphoreTake(mutex,0)!=pdTRUE) return false;
    if(inFlight) {xSemaphoreGive(mutex);return false;}
    inFlight=true; uint8_t signal=3; bool ok=xQueueSend(queue,&signal,0)==pdTRUE;
    if(!ok) inFlight=false;
    xSemaphoreGive(mutex);return ok;
}
bool NoteStorage::storageSnapshot(StorageStats& output) {
    if(!mutex || xSemaphoreTake(mutex,0)!=pdTRUE) return false;
    output=stats; xSemaphoreGive(mutex);return true;
}
bool NoteStorage::idle() {
    if(!mutex) return true;
    if(xSemaphoreTake(mutex,0)!=pdTRUE) return false;
    bool ready=!inFlight;xSemaphoreGive(mutex);return ready;
}
struct SdClock {uint32_t now(){return millis();}void yield(){vTaskDelay(1);}};
void NoteStorage::publish() {
    xSemaphoreTake(mutex,portMAX_DELAY);
    stats.mounted=working.mounted;
    stats.capacityValid=false;
    if(!stats.mounted) {stats.capacityValid=false;stats.total=stats.used=0;}
    ++stats.revision;
    working.revision=published.revision+1; working.busy=false; published=working; inFlight=false;
    xSemaphoreGive(mutex);
}
bool NoteStorage::mount(bool prepareNotes,bool force) {
    if(force || !mounted) {
        SD.end();
        static bool busStarted=false;
        if(!busStarted) { SPI.begin(40,39,14,12); busStarted=true; }
        mounted=SD.begin(12,SPI,10000000) && SD.cardType()!=CARD_NONE;
    }
    working.mounted=mounted;
    if(!working.mounted) { working.listValid=false; snprintf(working.message,sizeof(working.message),"SD unavailable; draft kept in RAM"); return false; }
    if(prepareNotes && !prepareNoteFolder(SD)) {
        mounted=false;working.listValid=false;
        snprintf(working.message,sizeof(working.message),"Cannot create note folder"); return false;
    }
    return true;
}
bool NoteStorage::list() {
    const unsigned previous=working.count;
    working.listValid=false;working.emptyVerified=false;working.rescanConfirmed=false;
    bool rescanned=false;
    bool ok=scanDirectory();
    // Reopen from a fresh mount after any failure, empty result or unexpected
    // shrink. Every scan closes its directory/files before the remount.
    if(!ok || scan.count==0 || scan.count<previous) {
        rescanned=true;
        Serial.printf("[notes] rescan previous=%u candidate=%u ok=%u\n",previous,scan.count,ok);
        ok=mount(true,true) && scanDirectory();
    }
    if(!ok) {mounted=false;return false;}
    if(!scan.count && scan.truncated){snprintf(working.message,sizeof(working.message),"Folder scan limited; retry");return false;}
    scan.commit(working,scan.count==0);
    working.rescanConfirmed=rescanned;
    Serial.printf("[notes] index count=%u truncated=%u\n",working.count,working.truncated);
    return true;
}
bool NoteStorage::scanDirectory() {
    SdClock clock;
    return scanNoteDirectory(SD,scan,operationStarted,clock,working.message,sizeof(working.message));
}
bool NoteStorage::read() {
    char path[64]; pathFor(path,sizeof(path),request.id);
    auto file=SD.open(path,"r");
    if(!file || file.isDirectory()) { snprintf(working.message,sizeof(working.message),"Cannot open note; file kept"); return false; }
    size_t size=file.size();
    if(size>NoteLimit) { file.close(); snprintf(working.message,sizeof(working.message),"Note exceeds 4 KB; file kept"); return false; }
    size_t read=file.read(reinterpret_cast<uint8_t*>(working.data),size); file.close();
    if(read!=size) { snprintf(working.message,sizeof(working.message),"Read failed; file kept"); return false; }
    working.data[size]=0;
    for(size_t i=0;i<size;++i)
        if(working.data[i]!='\n' && working.data[i]!='\r' && (uint8_t(working.data[i])<32 || uint8_t(working.data[i])>126)) {
            snprintf(working.message,sizeof(working.message),"Unsupported text; file kept"); return false;
        }
    strlcpy(working.id,request.id,sizeof(working.id)); snprintf(working.message,sizeof(working.message),"Opened"); return true;
}
bool NoteStorage::save() {
    char id[17]; strlcpy(id,request.id,sizeof(id));
    if(!*id) {
        if(!list()) return false;
        if(working.count==MaxNotes || working.truncated) { snprintf(working.message,sizeof(working.message),"Note limit reached; draft kept"); return false; }
        bool available=false;
        for(unsigned attempt=0;attempt<8;++attempt) {
            snprintf(id,sizeof(id),"%08lx%08lx",static_cast<unsigned long>(esp_random()),static_cast<unsigned long>(esp_random()));
            char path[64],backup[64],temp[64]; pathFor(path,sizeof(path),id); pathFor(backup,sizeof(backup),id,"bak"); pathFor(temp,sizeof(temp),id,"tmp");
            if(!SD.exists(path) && !SD.exists(backup) && !SD.exists(temp)) { available=true; break; }
        }
        if(!available) { snprintf(working.message,sizeof(working.message),"Cannot allocate note ID"); return false; }
    }
    char path[64],temporary[64],backup[64]; pathFor(path,sizeof(path),id);
    pathFor(temporary,sizeof(temporary),id,"tmp"); pathFor(backup,sizeof(backup),id,"bak");
    if(!commitNote(SD,path,temporary,backup,request.data,strlen(request.data))) {
        snprintf(working.message,sizeof(working.message),"Save failed; draft kept in RAM"); return false;
    }
    strlcpy(working.id,id,sizeof(working.id));
    bool listed=list(); // A verified commit is saved even if refresh fails.
    snprintf(working.message,sizeof(working.message),listed?"Saved to microSD":"Saved; list refresh pending"); return true;
}
bool NoteStorage::remove() {
    char path[64],backup[64],temporary[64]; pathFor(path,sizeof(path),request.id);
    pathFor(backup,sizeof(backup),request.id,"bak"); pathFor(temporary,sizeof(temporary),request.id,"tmp");
    char rem[64]; pathFor(rem,sizeof(rem),request.id,"rem");
    char rbak[64],rtmp[64]; pathFor(rbak,sizeof(rbak),request.id,"rbak"); pathFor(rtmp,sizeof(rtmp),request.id,"rtmp");
    if((SD.exists(rbak) && !SD.remove(rbak)) || (SD.exists(rtmp) && !SD.remove(rtmp)) || (SD.exists(rem) && !SD.remove(rem))) { snprintf(working.message,sizeof(working.message),"Reminder removal failed; retry"); return false; }
    // Remove recovery copies first so a deleted note cannot reappear on reboot.
    if((SD.exists(backup) && !SD.remove(backup)) || (SD.exists(temporary) && !SD.remove(temporary)) || !SD.remove(path)) {
        snprintf(working.message,sizeof(working.message),"Delete failed; retry"); return false;
    }
    bool listed=list(); snprintf(working.message,sizeof(working.message),listed?"Deleted":"Deleted; list refresh pending"); return true;
}
bool NoteStorage::reminder() {
    char path[64],temporary[64],backup[64],note[64];pathFor(note,sizeof(note),request.id);
    pathFor(path,sizeof(path),request.id,"rem");pathFor(temporary,sizeof(temporary),request.id,"rtmp");pathFor(backup,sizeof(backup),request.id,"rbak");
    if(request.operation==NoteOperation::ReminderAdvance) {
        NoteReminder expected,next;
        if(!decodeReminderAdvance(request.data,expected,next) || !advanceNoteReminder(SD,note,path,temporary,backup,expected,next)) {snprintf(working.message,sizeof(working.message),"Reminder update failed; retrying");return false;}
        bool ok=list();snprintf(working.message,sizeof(working.message),"Reminder schedule updated");return ok;
    }
    char date[17]{};memcpy(date,request.data,strlen(request.data)<16?strlen(request.data):16);
    unsigned repeat=0,active=1;int n=0;
    if(strlen(request.data)>16 && (sscanf(request.data+16,"|%u|%u%n",&repeat,&active,&n)!=2 || request.data[16+n] || repeat>3 || active>1)) {snprintf(working.message,sizeof(working.message),"Invalid reminder options");return false;}
    NoteReminder value;value.due=parseReminderDate(date);value.repeat=ReminderRepeat(repeat);value.active=active;
    if(value.active && value.repeat==ReminderRepeat::Weekdays && !reminderWeekday(value.due)){snprintf(working.message,sizeof(working.message),"Weekdays: choose a Monday-Friday");return false;}
    if(!validReminder(value) || !SD.exists(note)) {snprintf(working.message,sizeof(working.message),"Reminder needs a saved note/date");return false;}
    if(!saveNoteReminder(SD,path,temporary,backup,value)){snprintf(working.message,sizeof(working.message),"Reminder save failed");return false;}
    strlcpy(working.id,request.id,sizeof(working.id));
    if(!list()){snprintf(working.message,sizeof(working.message),"Reminder saved; refresh failed");return false;}
    for(unsigned i=0;i<working.count;++i)if(!strcmp(working.entries[i].id,request.id)) {
        const auto& entry=working.entries[i];
        if(entry.reminderAt==value.due && entry.repeat==value.repeat && entry.reminderActive==value.active){snprintf(working.message,sizeof(working.message),"Reminder saved locally");return true;}
        break;
    }
    snprintf(working.message,sizeof(working.message),"Reminder verification failed");return false;
}
void NoteStorage::taskEntry(void* context) { static_cast<NoteStorage*>(context)->run(); }
void NoteStorage::run() {
    uint8_t signal;
    while(true) {
        xQueueReceive(queue,&signal,portMAX_DELAY);
        operationStarted=millis();
        if(signal==2 || signal==3) {
            bool mounted=terminalSystem && signal==2?working.mounted:mount(false);
            StorageStats next; next.mounted=mounted;
            if(mounted && signal==3) { next.total=SD.totalBytes(); next.used=SD.usedBytes(); next.capacityValid=next.total>0; }
            if(signal==2) {
                terminalWorking=TerminalResult{};
                strlcpy(terminalWorking.cwd,terminalCwd,sizeof(terminalWorking.cwd));
                if(terminalSystem) executeSystem(terminalCommand,terminalCwd,terminalWorking);
                else if(mounted || !strcmp(terminalCommand,"help") || !strcmp(terminalCommand,"pwd")) {SdClock clock; executeSd(SD,terminalCommand,terminalCwd,terminalConfirmed,terminalWorking,clock);}
                else terminalWorking.fail("SD card unavailable");
                if(!terminalWorking.ok && !terminalSystem)mounted=false;
                if(terminalWorking.mutated) { working.operation=NoteOperation::List; working.ok=list(); }
            }
            xSemaphoreTake(mutex,portMAX_DELAY);
            next.revision=stats.revision+1;stats=next;
            if(signal==2) {
                terminalWorking.revision=terminalPublished.revision+1; terminalPublished=terminalWorking;
                if(terminalWorking.mutated) {working.revision=published.revision+1;working.busy=false;published=working;}
            }
            inFlight=false; xSemaphoreGive(mutex); continue;
        }
        uint32_t started=millis();
        Serial.printf("[notes] begin op=%u heap=%u stack=%u\n",unsigned(request.operation),ESP.getFreeHeap(),unsigned(uxTaskGetStackHighWaterMark(nullptr)));
        working.operation=request.operation; working.ok=false;
        if(mount()) {
            switch(request.operation) {
            case NoteOperation::List:working.ok=list();break;
            case NoteOperation::Read:working.ok=read();break;
            case NoteOperation::Save:working.ok=save();break;
            case NoteOperation::Delete:working.ok=remove();break;
            case NoteOperation::Reminder:
            case NoteOperation::ReminderAdvance:working.ok=reminder();break;
            }
        }
        Serial.printf("[notes] end op=%u ok=%u ms=%u heap=%u stack=%u\n",unsigned(request.operation),working.ok,unsigned(millis()-started),ESP.getFreeHeap(),unsigned(uxTaskGetStackHighWaterMark(nullptr)));
        if(!working.ok)mounted=false;
        memset(request.data,0,sizeof(request.data)); publish();
    }
}
}
