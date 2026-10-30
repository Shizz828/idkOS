#include <cassert>
#include "../src/apps/CaptureApp.h"
struct Store {
    spark::NoteStorageState result;bool accept=true;unsigned saves=0;
    char saved[spark::NoteLimit+1]{};
    bool submit(spark::NoteOperation op,const char* ="",const char* data=""){
        if(!accept || result.busy)return false;
        result.operation=op;result.busy=true;result.started=100;++result.revision;
        if(op==spark::NoteOperation::Save){strcpy(saved,data);++saves;}return true;
    }
    bool snapshot(spark::NoteStorageState& out){out=result;return true;}
    void finish(bool ok){
        result.busy=false;result.ok=ok;++result.revision;
        strcpy(result.message,ok?"Saved to microSD":"Save failed; draft kept in RAM");
        if(ok){result.count=1;result.listValid=true;strcpy(result.id,"0000000000000001");strcpy(result.entries[0].id,result.id);strcpy(result.entries[0].title,"My unsaved note");}
    }
};
bool shows(const char* value){for(const auto& row:M5Cardputer.Display.printed)if(row.text==value)return true;return false;}
int main(){
    using namespace spark;
    Store store;CaptureApp<Store> app;InputEvent event;
    event.text[0]='n';app.input(event,store);event={};strcpy(event.text,"My unsaved note");app.input(event,store);
    assert(!app.leave(false,store));app.draw();assert(shows("Unsaved changes") && shows("G0 Home keeps draft in RAM"));
    // Ctrl+S refusal was previously invisible on the discard-confirmation page.
    store.accept=false;event={};event.save=true;app.input(event,store);app.draw();
    assert(shows("Storage busy; draft kept") && store.saves==0);
    store.accept=true;app.input(event,store);app.draw();assert(shows("Saving... G0 keeps draft"));
    store.finish(false);app.poll(store,200);app.draw();assert(shows("Save failed; draft kept in RAM"));
    // Retrying from this same modal saves the original draft and closes cleanly.
    app.input(event,store);assert(store.saves==2 && !strcmp(store.saved,"My unsaved note"));
    store.finish(true);app.poll(store,300);app.draw();assert(!app.textFocused() && shows("My unsaved note"));
    // A second unsaved draft remains accessible through G0 even if save stalls.
    event={};event.text[0]='n';app.input(event,store);event={};strcpy(event.text,"Keep this draft");app.input(event,store);
    assert(!app.leave(false,store));event={};event.save=true;app.input(event,store);app.poll(store,400);
    app.poll(store,15100);app.draw();assert(shows("SD timeout; draft kept; G0 exits"));
    assert(app.leave(true,store));app.open(store);assert(app.textFocused());
}
