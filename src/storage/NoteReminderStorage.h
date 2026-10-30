#pragma once
#include "NoteTransaction.h"
#include "../core/NoteReminder.h"
namespace spark {
template<typename Filesystem>
bool loadNoteReminder(Filesystem& fs,const char* path,NoteReminder& out) {
    auto file=fs.open(path,"r");if(!file || file.isDirectory() || file.size()>=64){file.close();return false;}
    char data[64]{};size_t size=file.size();bool ok=file.read(reinterpret_cast<uint8_t*>(data),size)==int(size);file.close();
    return ok && decodeNoteReminder(data,out);
}
template<typename Filesystem>
bool saveNoteReminder(Filesystem& fs,const char* path,const char* temporary,const char* backup,const NoteReminder& value) {
    if(!validReminder(value))return false;
    char data[64];encodeNoteReminder(value,data,sizeof(data));
    if(!commitNote(fs,path,temporary,backup,data,strlen(data)))return false;
    // Confirm the final sidecar can be decoded by the list loader before the
    // caller reports success or displays the reminder marker.
    NoteReminder saved;return loadNoteReminder(fs,path,saved) && sameReminder(value,saved);
}
// Compare-and-save prevents a delayed delivery update from overriding a user edit
// or recreating metadata for a deleted note. A conflict is handled by re-listing.
template<typename Filesystem>
bool advanceNoteReminder(Filesystem& fs,const char* note,const char* path,const char* temporary,const char* backup,const NoteReminder& expected,const NoteReminder& next) {
    if(!fs.exists(note))return true;
    if(!recoverNote(fs,path,backup))return false;
    if(!fs.exists(path))return true;
    NoteReminder current;if(!loadNoteReminder(fs,path,current))return false;
    if(!sameReminder(current,expected))return true;
    return saveNoteReminder(fs,path,temporary,backup,next);
}
}
