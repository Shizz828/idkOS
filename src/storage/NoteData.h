#pragma once
#include "../core/NoteEditor.h"
#include "../core/NoteReminder.h"
namespace spark {
constexpr unsigned MaxNotes=64;
template<typename Reader>
unsigned readNoteTitle(Reader& note,char* output,unsigned capacity) {
    unsigned n=0,bytes=0;
    while(note.available() && n+1<capacity && bytes++<256) {
        int c=note.read(); if(c<0 || c=='\n') break;
        if(c>=32 && c<=126) output[n++]=char(c);
    }
    output[n]=0; return n;
}
enum class NoteOperation : uint8_t { List, Read, Save, Delete, Reminder, ReminderAdvance };
struct NoteEntry { char id[17]{}; char title[33]{}; int64_t reminderAt=0; ReminderRepeat repeat=ReminderRepeat::Once; bool reminderActive=true; int reminderHour=-1,reminderMinute=-1; };
struct NoteStorageState {
    NoteEntry entries[MaxNotes];
    unsigned count=0;
    bool busy=false,ok=false,mounted=false,truncated=false,listValid=false;
    bool emptyVerified=false; // RAM only: a fresh second scan confirmed an empty directory.
    bool rescanConfirmed=false; // A suspicious shrink was checked with a fresh mount/scan.
    NoteOperation operation=NoteOperation::List;
    char id[17]{}, data[NoteLimit+1]{}, message[40]="Enter to mount SD";
    uint32_t revision=0,started=0;
};
}
