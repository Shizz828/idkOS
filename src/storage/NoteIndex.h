#pragma once
#include "NoteData.h"
#include <ctype.h>
namespace spark {
inline bool validNoteIndex(const NoteStorageState& state) {
    if(state.count>MaxNotes)return false;
    for(unsigned i=0;i<state.count;++i) {
        const auto& entry=state.entries[i];
        if(strnlen(entry.id,sizeof(entry.id))!=16 || !memchr(entry.title,0,sizeof(entry.title)))return false;
        for(unsigned c=0;c<16;++c)if(!isxdigit(static_cast<unsigned char>(entry.id[c])))return false;
        for(unsigned j=0;j<i;++j)if(!strcmp(entry.id,state.entries[j].id))return false;
    }
    return true;
}
// The scanner writes into scratch space. Failed/partial scans cannot replace
// the last complete index or remove scheduled reminders from memory.
class NoteIndexScan {
public:
    NoteEntry entries[MaxNotes];
    unsigned count=0;bool truncated=false;
    void reset(){count=0;truncated=false;}
    void commit(NoteStorageState& target,bool emptyVerified)const {
        memcpy(target.entries,entries,count*sizeof(NoteEntry));
        target.count=count;target.truncated=truncated;
        target.listValid=true;target.emptyVerified=emptyVerified;
    }
};
}
