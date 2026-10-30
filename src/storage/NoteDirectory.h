#pragma once
#include "NoteIndex.h"
#include "NoteReminderStorage.h"
namespace spark {
inline bool validScanId(const char* id) {
    if(strlen(id)!=16)return false;
    for(unsigned i=0;i<16;++i)if(!isxdigit(static_cast<unsigned char>(id[i])))return false;
    return true;
}
inline void noteScanPath(char* out,size_t size,const char* id,const char* extension="txt") {
    snprintf(out,size,"/idkos/notes/%s.%s",id,extension);
}
// Fresh cards get /idkos/notes. A card from earlier builds has its old
// folder moved once; reminders survive because NVS stores note IDs, not paths.
// A failed move leaves /spark/notes untouched and is retried on the next mount.
template<typename Filesystem>
bool prepareNoteFolder(Filesystem& fs) {
    if(fs.exists("/idkos/notes")) return true;
    if(!fs.exists("/idkos") && !fs.mkdir("/idkos")) return false;
    if(fs.exists("/spark/notes")) return fs.rename("/spark/notes","/idkos/notes");
    return fs.mkdir("/idkos/notes");
}
template<typename Filesystem,typename Clock>
bool scanNoteDirectory(Filesystem& fs,NoteIndexScan& scan,uint32_t operationStarted,
                       Clock& clock,char* message,size_t capacity) {
    scan.reset();
    auto directory=fs.open("/idkos/notes","r");
    if(!directory || !directory.isDirectory()) { directory.close();snprintf(message,capacity,"Cannot read note folder"); return false; }
    unsigned scanned=0;
    while(true) {
        if(uint32_t(clock.now()-operationStarted)>15000) { directory.close(); snprintf(message,capacity,"Directory timeout; retry"); return false; }
        auto file=directory.openNextFile(); if(!file) break;
        char name[64];snprintf(name,sizeof(name),"%s",file.name());
        bool folder=file.isDirectory();file.close();
        const char* leaf=strrchr(name,'/');if(leaf)memmove(name,leaf+1,strlen(leaf+1)+1);
        if(++scanned>512) { scan.truncated=true; break; }
        if(folder || strlen(name)!=20) continue;
        char id[17]{}; memcpy(id,name,16); if(!validScanId(id)) continue;
        if(!strcmp(name+16,".bak")) {
            char path[64],backup[64]; noteScanPath(path,sizeof(path),id); noteScanPath(backup,sizeof(backup),id,"bak");
            if(fs.exists(path)) continue;
            if(!recoverNote(fs,path,backup)) { directory.close(); snprintf(message,capacity,"Recovery copy could not be restored"); return false; }
            snprintf(name,sizeof(name),"%s.txt",id);
        }
        if(strcmp(name+16,".txt")) continue;
        bool duplicate=false;
        for(unsigned i=0;i<scan.count;++i) if(!strcmp(scan.entries[i].id,id)) duplicate=true;
        if(duplicate) continue;
        if(scan.count==MaxNotes) { scan.truncated=true; continue; }
        auto& entry=scan.entries[scan.count++]; strlcpy(entry.id,id,sizeof(entry.id));
        char path[64]; noteScanPath(path,sizeof(path),id);
        auto note=fs.open(path,"r"); unsigned n=0;
        if(!note) { directory.close(); snprintf(message,capacity,"SD read failed; retry"); return false; }
        n=readNoteTitle(note,entry.title,sizeof(entry.title));
        entry.title[n]=0; if(!n) strlcpy(entry.title,"Untitled",sizeof(entry.title)); note.close();
        entry.reminderAt=0;entry.repeat=ReminderRepeat::Once;entry.reminderActive=true;entry.reminderHour=entry.reminderMinute=-1;
        char metadata[64]; noteScanPath(metadata,sizeof(metadata),id,"rem");
        char recovery[64]; noteScanPath(recovery,sizeof(recovery),id,"rbak");
        if(!recoverNote(fs,metadata,recovery)) { directory.close(); snprintf(message,capacity,"Reminder recovery failed; retry"); return false; }
        NoteReminder saved;
        if(loadNoteReminder(fs,metadata,saved)) {entry.reminderAt=saved.due;entry.repeat=saved.repeat;entry.reminderActive=saved.active;entry.reminderHour=saved.hour;entry.reminderMinute=saved.minute;}
        if(scanned%8==0) clock.yield();
    }
    directory.close();
    snprintf(message,capacity,scan.truncated?"List limited to 64 notes":"Notes on microSD"); return true;
}
}
