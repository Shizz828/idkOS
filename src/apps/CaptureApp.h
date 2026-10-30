#pragma once
#include "../core/NoteEditor.h"
#include "../storage/NoteData.h"
#include "../storage/NoteIndex.h"
#include "../core/Input.h"
#include "../ui/Screen.h"
#include "../core/ReminderEngine.h"
#include "../core/SystemTime.h"
#include "../core/ClockFormat.h"
#include "../core/ReminderForm.h"
namespace spark {
template<typename Storage>
class CaptureApp {
    enum class Confirm { None, Discard, Delete } confirm=Confirm::None;
    NoteStorageState state,incoming;
    bool needsReload=true;
    uint32_t reloadDue=0;
    NoteEditor editor;
    char id[17]{}, message[40]="N: new  Enter: open";
    unsigned selected=0;
    uint32_t revision=0,savedGeneration=0,session=0,savedSession=0;
    bool timedOut=false,reminderEditing=false,reminderSaving=false;
    ReminderForm reminder;
    bool confirmArmed=true;
    bool editing=false,pending=false,closeAfterSave=false;
    void refresh(Storage& storage) {
        needsReload=true;
        pending=storage.submit(NoteOperation::List);
        if(pending) timedOut=false;
        if(!pending) snprintf(message,sizeof(message),"Storage busy; Enter to retry");
    }
    bool recoverList(Storage& storage,uint32_t now,bool visible) {
        if(!visible || editing || reminderEditing || confirm!=Confirm::None || pending ||
           state.busy || !needsReload || int32_t(now-reloadDue)<0)return false;
        refresh(storage);reloadDue=now+5000;
        return true;
    }
    void save(Storage& storage) {
        pending=storage.submit(NoteOperation::Save,id,editor.data);
        if(pending) { savedGeneration=editor.generation; savedSession=session; timedOut=false; }
        if(!pending) closeAfterSave=false;
        snprintf(message,sizeof(message),pending ? "Saving..." : "Storage busy; draft kept");
    }
public:
    bool textFocused() const { return editing || reminderEditing || confirm!=Confirm::None; }
    // The reminder form uses menu navigation (plain arrows, Enter), not text entry.
    bool reminderForm() const { return reminderEditing; }
    void open(Storage& storage) { if(!editing && !pending) refresh(storage); }
    bool poll(Storage& storage,uint32_t now=0,bool visible=true) {
        if(!storage.snapshot(incoming)) return false;
        // Busy/failed/malformed snapshots carry operation results, not a new
        // authoritative list. Keep the previous index until a complete scan.
        bool valid=incoming.listValid && validNoteIndex(incoming) && (incoming.count || incoming.emptyVerified) &&
            (incoming.count>=state.count || incoming.rescanConfirmed || incoming.emptyVerified);
        if(valid && !incoming.busy) {
            memcpy(state.entries,incoming.entries,incoming.count*sizeof(NoteEntry));
            state.count=incoming.count;state.truncated=incoming.truncated;
        }
        state.ok=incoming.ok;state.busy=incoming.busy;state.mounted=incoming.mounted;
        state.listValid=valid;state.emptyVerified=incoming.emptyVerified;
        state.rescanConfirmed=incoming.rescanConfirmed;
        state.operation=incoming.operation;state.revision=incoming.revision;state.started=incoming.started;
        memcpy(state.id,incoming.id,sizeof(state.id));memcpy(state.data,incoming.data,sizeof(state.data));
        memcpy(state.message,incoming.message,sizeof(state.message));
        bool changed=state.revision!=revision; revision=state.revision;
        if(!state.busy && !valid)needsReload=true;
        if(state.busy) {
            if(!timedOut && now && uint32_t(now-state.started)>=15000) {
                timedOut=true; pending=false; closeAfterSave=false;reminderSaving=false;
                snprintf(message,sizeof(message),"SD timeout; draft kept; G0 exits");
                return true;
            }
            return changed;
        }
        if(!changed) return recoverList(storage,now,visible);
        pending=false; strlcpy(message,state.message,sizeof(message));
        needsReload=!valid || (!state.count && !state.emptyVerified);
        if(state.ok && !timedOut && state.operation==NoteOperation::Read) {
            if(editor.load(state.data,strlen(state.data))) { strlcpy(id,state.id,sizeof(id)); editing=true; ++session; }
            else snprintf(message,sizeof(message),"Unsupported note; original kept");
        }
        if(state.ok && state.operation==NoteOperation::Save && session==savedSession) {
            strlcpy(id,state.id,sizeof(id)); editor.dirty=editor.generation!=savedGeneration;
            if(editor.dirty) snprintf(message,sizeof(message),"Earlier draft saved; edits pending");
            else snprintf(message,sizeof(message),"Saved; Ctrl+R adds a reminder");
            for(unsigned i=0;i<state.count;++i) if(!strcmp(state.entries[i].id,id)) selected=i;
            if(closeAfterSave) { editing=false; confirm=Confirm::None; }
        }
        if(reminderSaving && state.operation==NoteOperation::Reminder) {
            reminderSaving=false;
            if(state.ok && !timedOut && !strcmp(state.id,id)) {
                reminderEditing=false;editing=false;
                for(unsigned i=0;i<state.count;++i)if(!strcmp(state.entries[i].id,id)){selected=i;break;}
            }
        }
        closeAfterSave=false; timedOut=false;
        if(selected>=state.count) selected=state.count ? state.count-1 : 0;
        recoverList(storage,now,visible);
        return true;
    }
    // Home keeps drafts in memory. Back explicitly confirms any discard.
    bool leave(bool home,Storage& storage) {
        if(reminderEditing) {
            if(pending)return home;
            if(!home && reminder.back())return false; // Esc undoes only the field being edited.
            reminderEditing=false; return home;
        }
        if(home) { confirm=Confirm::None; return true; }
        if(confirm!=Confirm::None) { confirm=Confirm::None; return false; }
        if(editing) {
            if(pending) return false;
            if(editor.dirty) {confirm=Confirm::Discard;snprintf(message,sizeof(message),"Draft kept until saved or discarded");}
            else { editing=false; refresh(storage); }
            return false;
        }
        return !pending;
    }
    void input(const InputEvent& e,Storage& storage) {
        if(pending) return;
        if(reminderEditing) {
            if(reminder.input(e)==ReminderForm::Action::Save) {
                int64_t due=reminder.due(); int64_t now=time(nullptr);
                if(reminder.active && !systemTimeReady(now))snprintf(message,sizeof(message),"Waiting for time synchronization");
                else if(!due || (reminder.active && due<=now)) snprintf(message,sizeof(message),"Choose a future time and date");
                else if(reminder.active && reminder.repeat==ReminderRepeat::Weekdays && !reminderWeekday(due))snprintf(message,sizeof(message),"Weekdays: choose a Monday-Friday");
                else {
                    char stored[24];reminder.stored(stored,sizeof(stored));
                    char options[32];snprintf(options,sizeof(options),"%s|%u|%u",stored,unsigned(reminder.repeat),reminder.active?1:0);
                    pending=storage.submit(NoteOperation::Reminder,id,options);
                    if(pending) { reminderSaving=true; timedOut=false; }
                    else snprintf(message,sizeof(message),"Storage busy; Enter to retry");
                }
            }
            return;
        }
        if(e.reminder) {
            if(editing && (editor.dirty || !*id)) { snprintf(message,sizeof(message),"Save note before setting reminder"); return; }
            if(!editing) {
                if(!state.count) return;
                strlcpy(id,state.entries[selected].id,sizeof(id));
            }
            reminderEditing=true;
            int64_t due=0;ReminderRepeat repeat=ReminderRepeat::Once;bool active=true;
            for(unsigned i=0;i<state.count;++i)if(!strcmp(state.entries[i].id,id)) {const auto& entry=state.entries[i];due=entry.reminderAt;repeat=entry.repeat;if(due)active=entry.reminderActive;break;}
            // An existing reminder keeps every value; a new one starts an hour from now.
            reminder.load(due?due:systemTimeReady(time(nullptr))?time(nullptr)+3600:0,repeat,active);
            snprintf(message,sizeof(message),"New York time");return;
        }
        if(confirm==Confirm::Delete) {
            if(!confirmArmed) { if(!e.enterDown && !e.enter) confirmArmed=true; return; }
            if(e.enter && state.count) {
                pending=storage.submit(NoteOperation::Delete,state.entries[selected].id);
                if(pending) timedOut=false;
                confirm=Confirm::None;
                if(!pending) snprintf(message,sizeof(message),"Storage busy; delete canceled");
            }
            return;
        }
        if(confirm==Confirm::Discard) {
            if(e.save) { closeAfterSave=true; save(storage); }
            else if(e.enter) { editor.clear(); editing=false; confirm=Confirm::None; refresh(storage); }
            return;
        }
        if(editing) {
            if(e.save) { save(storage); return; }
            if(e.left) editor.horizontal(-1);
            if(e.right) editor.horizontal(1);
            if(e.up) editor.vertical(-1);
            if(e.down) editor.vertical(1);
            if(e.erase && editor.cursor) { editor.erase(); snprintf(message,sizeof(message),"Unsaved changes; Ctrl+S saves"); }
            if(e.enter) {
                bool inserted=editor.insert('\n');
                snprintf(message,sizeof(message),inserted?"Unsaved changes; Ctrl+S saves":"4 KB limit; save or edit");
            }
            for(const char* p=e.text;*p;++p) {
                bool inserted=editor.insert(*p);
                snprintf(message,sizeof(message),inserted?"Unsaved changes; Ctrl+S saves":"4 KB limit; save or edit");
            }
        } else {
            for(const char* p=e.text;*p;++p) if(*p=='n' || *p=='N') {
                editor.clear(); id[0]=0; editing=true; ++session;
                snprintf(message,sizeof(message),"RAM draft; Ctrl+S saves to SD"); return;
            }
            if(e.up && selected) --selected;
            if(e.down && selected+1<state.count) ++selected;
            if(e.left) selected=selected>=3 ? selected-3 : 0;
            if(e.right && state.count) selected=selected+3<state.count ? selected+3 : state.count-1;
            if(e.erase && state.count) { confirm=Confirm::Delete; confirmArmed=!e.enter && !e.enterDown; return; }
            if(e.enter) {
                if(state.count) { pending=storage.submit(NoteOperation::Read,state.entries[selected].id); if(pending) timedOut=false; }
                else refresh(storage);
            }
        }
    }
    const NoteStorageState& storageState() const { return state; }
    void draw() const {
        frame("CACHE");
        if(reminderEditing) {
            text(145,17,"NOTE REMINDER",1,theme::Attention);
            // The reminder's text is the note itself; edit it in the note.
            for(unsigned i=0;i<state.count;++i)if(!strcmp(state.entries[i].id,id)){char title[38];snprintf(title,sizeof(title),"%.37s",state.entries[i].title);text(6,31,title);break;}
            const char* labels[]={"Time","Date","Repeat","Active","Save reminder"};
            for(unsigned f=0;f<ReminderForm::FieldCount;++f) {
                int y=45+int(f)*14;bool chosen=f==reminder.field;
                text(6,y,chosen?">":" ",1,theme::Primary);text(18,y,labels[f],1,chosen?theme::Primary:theme::Content);
                char value[20]="";int start[3]{},length[3]{};
                if(f<=ReminderForm::Date)reminder.text(f,value,sizeof(value),start,length);
                else if(f==ReminderForm::Repeat)snprintf(value,sizeof(value),"%s",repeatName(reminder.repeat));
                else if(f==ReminderForm::Active)snprintf(value,sizeof(value),"%s",reminder.active?"Yes":"No");
                uint16_t color=f==ReminderForm::Active?(reminder.active?theme::Success:theme::Attention):chosen && reminder.editing?theme::Attention:theme::Content;
                text(84,y,value,1,color);
                if(chosen && reminder.editing)screen().drawFastHLine(84+start[reminder.part]*6,y+9,length[reminder.part]*6,theme::Attention);
            }
            text(6,115,message,1,theme::Attention);
            text(6,126,reminder.editing?"L/R part  Up/Dn or type  Enter done":"Up/Dn move  Enter select  Esc cancel");return;
        }
        if(confirm!=Confirm::None) {
            text(6,36,confirm==Confirm::Delete?"Delete this note?":"Unsaved changes",2,confirm==Confirm::Delete?theme::Error:theme::Attention);
            if(confirm==Confirm::Delete) {
                text(6,62,state.entries[selected].title);
                text(6,88,"Enter: delete  Esc: cancel",1,theme::Error);
            } else {
                text(6,62,"Ctrl+S: save and close");
                text(6,78,"Enter: discard edits");
                text(6,94,"Esc: continue editing");
                // Save requests can fail or time out while this modal is open.
                // Keep their status and the non-destructive Home exit visible.
                text(6,112,pending?"Saving... G0 keeps draft":message,1,
                    strstr(message,"failed") || strstr(message,"busy") || strstr(message,"timeout")?theme::Error:theme::Attention);
                text(6,126,"G0 Home keeps draft in RAM");
            }
            return;
        }
        if(editing) {
            text(145,17,editor.dirty?"UNSAVED":*id?"SAVED":"RAM DRAFT",1,editor.dirty || !*id?theme::Attention:theme::Success);
            NotePosition p;
            char lines[NoteVisibleRows][NoteColumns+1]{};
            for(size_t i=0;i<editor.length;++i) {
                if(editor.data[i]!='\n' && p.row>=editor.firstRow && p.row<editor.firstRow+NoteVisibleRows)
                    lines[p.row-editor.firstRow][p.column]=editor.data[i];
                advanceNote(p,editor.data[i],i?editor.data[i-1]:0,i);
            }
            for(unsigned row=0;row<NoteVisibleRows;++row) text(6,31+row*16,lines[row],2);
            auto cursor=editor.position(editor.cursor);
            screen().drawFastHLine(6+cursor.column*12,31+(cursor.row-editor.firstRow)*16+14,10,theme::Primary);
            text(6,114,pending?"Saving...":message,1,strstr(message,"failed") || strstr(message,"timeout")?theme::Error:editor.dirty?theme::Attention:*id?theme::Success:theme::Content);
            text(6,126,"Ctrl+S save  Ctrl+R reminder  G0");
        } else {
            if(!state.count) {
                text(6,38,pending || needsReload?"Checking notes...":"No notes yet",2);
                text(6,66,"N: create a RAM draft");
                text(6,82,"Enter: retry microSD");
            }
            unsigned start=selected/3*3;
            // Saving selects the new note; show which page is visible so notes
            // on other pages are not mistaken for missing ones.
            if(state.count>3) {
                char page[24];snprintf(page,sizeof(page),"%u-%u of %u",start+1,start+3<state.count?start+3:state.count,state.count);
                text(145,17,page,1,theme::Attention);
            }
            for(unsigned row=0;row<3 && start+row<state.count;++row) {
                unsigned i=start+row; int y=33+row*24;
                text(6,y,i==selected?">":" ",2);
                text(23,y+4,state.entries[i].title);
                if(state.entries[i].reminderAt) text(221,y+4,"R",1,theme::Attention);
            }
            char status[38]{}; strlcpy(status,pending?"Loading microSD...":message,sizeof(status)); text(6,106,status,1,strstr(status,"failed") || strstr(status,"timeout") || strstr(status,"unavailable")?theme::Error:!strcmp(status,"Deleted")?theme::Success:theme::Content);
            text(6,126,"N new  Enter open  Ctrl+R reminder");
        }
    }
};
}
