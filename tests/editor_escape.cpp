#include <cassert>
#include "../src/apps/CaptureApp.h"
#include "../src/core/ScreenSleep.h"
struct Store {
    spark::NoteStorageState result;
    char saved[spark::NoteLimit+1]{};unsigned saves=0;
    bool submit(spark::NoteOperation op,const char* ="",const char* data=""){
        if(result.busy)return false;
        result.busy=true;result.started=10;result.operation=op;++result.revision;
        if(op==spark::NoteOperation::Save){strcpy(saved,data);++saves;}
        return true;
    }
    bool snapshot(spark::NoteStorageState& out){out=result;return true;}
};
int main(){
    using namespace spark;
    Store store;CaptureApp<Store> app;InputEvent event;
    event.text[0]='n';app.input(event,store);
    event={};strcpy(event.text,"ADV case draft\nKeep my notes");app.input(event,store);
    event={};event.save=true;app.input(event,store);assert(store.saves==1);
    app.poll(store,100);app.draw();
    ScreenSleep sleep;bool wake=false;
    sleep.filter(false,false,false,false,wake);
    sleep.filter(true,true,false,true,wake);
    sleep.filter(false,false,false,false,wake);
    assert(sleep.filter(false,true,false,true,wake) && !wake); // Keys cannot wake.
    assert(sleep.filter(false,false,true,true,wake,true) && wake); // G0 wake is consumed.
    for(unsigned i=0;i<10;++i)assert(sleep.filter(false,true,false,true,wake));
    assert(!sleep.filter(false,false,true,true,wake)); // Fresh G0 bypasses a stuck release gate.
    assert(app.leave(true,store)); // Home works even while SD save remains busy.
    app.open(store);assert(app.textFocused());
    assert(app.poll(store,15010)); // Timeout is responsive and keeps the editor draft.
    assert(app.leave(true,store));app.open(store);
    store.result.busy=false;store.result.ok=false;++store.result.revision;
    app.poll(store,15020);
    event={};event.text[0]='!';app.input(event,store);
    event={};event.save=true;app.input(event,store);
    assert(store.saves==2 && !strcmp(store.saved,"ADV case draft\nKeep my notes!"));
}
