#pragma once
#include <Preferences.h>
#include "../core/ReminderEngine.h"
#include <stddef.h>
#include <memory>
#include <new>
namespace spark {
class ReminderService {
    // Persist only populated schedules; normal notes consume no reminder NVS space.
    struct Blob {uint32_t version=2;ReminderNotice notice;int64_t lastWorkSlot=0;uint32_t count=0;ScheduledReminder items[MaxNotes];uint32_t checksum=0;};
    static uint32_t byteChecksum(const void* data,size_t size) {uint32_t hash=2166136261u;const auto* bytes=static_cast<const uint8_t*>(data);for(size_t i=0;i<size;++i)hash=(hash^bytes[i])*16777619u;return hash;}
    struct LegacyItem {char id[17]{},text[33]{};int64_t due=0,delivered=0;};
    struct LegacyRecord {uint32_t version=1;LegacyItem items[MaxNotes];ReminderNotice notice;int64_t lastWorkSlot=0;};
    struct LegacyBlob {LegacyRecord record;uint32_t checksum=0;};
    uint32_t lastStorageAttempt=0;int64_t lastSaveAttempt=0;
    template<typename Value> static uint32_t checksum(const Value& b) {
        uint32_t h=2166136261u; auto bytes=reinterpret_cast<const uint8_t*>(&b);
        for(size_t i=0;i<offsetof(Value,checksum);++i) h=(h^bytes[i])*16777619u;
        return h;
    }
public:
    ReminderEngine engine;
    bool saveFailed=false;
    void begin() {
        Preferences prefs; std::unique_ptr<Blob> blob(new(std::nothrow) Blob{});
        if(!blob){saveFailed=true;return;}
        if(prefs.begin("spark-reminders",true)) {
            size_t size=prefs.getBytesLength("state");bool ok=false;
            if(size>=offsetof(Blob,items)+sizeof(uint32_t) && size<=sizeof(Blob) && prefs.getBytes("state",blob.get(),size)==size && blob->version==2 && blob->count<=MaxNotes && size==offsetof(Blob,items)+blob->count*sizeof(ScheduledReminder)+sizeof(uint32_t)) {
                uint32_t hash=0;memcpy(&hash,reinterpret_cast<uint8_t*>(blob.get())+size-sizeof(hash),sizeof(hash));ok=hash==byteChecksum(blob.get(),size-sizeof(hash));
            }
            if(ok){engine.record.notice=blob->notice;engine.record.lastWorkSlot=blob->lastWorkSlot;for(unsigned i=0;i<blob->count;++i)engine.record.items[i]=blob->items[i];}
            else if(prefs.getBytesLength("state")==sizeof(LegacyBlob)) {
                blob.reset();std::unique_ptr<LegacyBlob> old(new(std::nothrow) LegacyBlob{});
                if(old && prefs.getBytes("state",old.get(),sizeof(LegacyBlob))==sizeof(LegacyBlob) && old->record.version==1 && old->checksum==checksum(*old)) {
                    engine.record.notice=old->record.notice;engine.record.lastWorkSlot=old->record.lastWorkSlot;
                    for(unsigned i=0;i<MaxNotes;++i){const auto& source=old->record.items[i];auto& target=engine.record.items[i];strlcpy(target.id,source.id,sizeof(target.id));strlcpy(target.text,source.text,sizeof(target.text));target.due=source.due;target.delivered=source.delivered;target.active=source.delivered!=source.due;
                        if(source.id[0] && !target.active){target.pending=true;target.previous.due=source.due;target.previous.active=true;}}
                }
            }
            prefs.end();
        }
    }
    void save() {
        std::unique_ptr<Blob> blob(new(std::nothrow) Blob{});
        if(!blob){saveFailed=true;return;}
        blob->notice=engine.record.notice;blob->lastWorkSlot=engine.record.lastWorkSlot;
        for(const auto& item:engine.record.items)if(item.id[0])blob->items[blob->count++]=item;
        size_t size=offsetof(Blob,items)+blob->count*sizeof(ScheduledReminder)+sizeof(uint32_t);
        uint32_t hash=byteChecksum(blob.get(),size-sizeof(hash));memcpy(reinterpret_cast<uint8_t*>(blob.get())+size-sizeof(hash),&hash,sizeof(hash));
        Preferences prefs; saveFailed=true;
        if(prefs.begin("spark-reminders",false)) { saveFailed=prefs.putBytes("state",blob.get(),size)!=size; prefs.end(); }
    }
    bool poll(int64_t now,bool synchronized=true) {
        bool wasWaiting=engine.waitingForTime,wasFailed=saveFailed;
        bool changed=engine.tick(now,synchronized);
        if(changed || (saveFailed && now-lastSaveAttempt>=10)){lastSaveAttempt=now;save();}
        return changed || wasWaiting!=engine.waitingForTime || wasFailed!=saveFailed;
    }
    template<typename Storage> void flush(Storage& storage,uint32_t now) {
        if(saveFailed || (lastStorageAttempt && uint32_t(now-lastStorageAttempt)<5000))return;
        for(const auto& item:engine.record.items)if(item.id[0] && item.pending){
            NoteReminder next;next.due=item.due;next.repeat=item.repeat;next.active=item.active;next.hour=item.hour;next.minute=item.minute;
            char update[96];encodeReminderAdvance(item.previous,next,update,sizeof(update));
            if(storage.submit(NoteOperation::ReminderAdvance,item.id,update))lastStorageAttempt=now;
            return;
        }
    }
    void sync(const NoteStorageState& state) { if(engine.sync(state)) save(); }
};
}
