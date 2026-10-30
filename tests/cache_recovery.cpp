#include <cassert>
#include "../src/apps/CaptureApp.h"
struct Store {
    spark::NoteStorageState state;unsigned lists=0;
    bool accept=true;
    bool submit(spark::NoteOperation op,const char* ="",const char* =""){
        if(!accept)return false;
        if(op==spark::NoteOperation::List)++lists;
        state.operation=op;state.busy=true;++state.revision;return true;
    }
    bool snapshot(spark::NoteStorageState& out){out=state;return true;}
    void finish(unsigned count,bool valid=true,bool ok=true,bool empty=false){
        state.count=count;state.listValid=valid;state.ok=ok;state.emptyVerified=empty;
        state.busy=false;++state.revision;
        for(unsigned i=0;i<count;++i){snprintf(state.entries[i].id,17,"%016x",i+1);snprintf(state.entries[i].title,33,"Note %u",i+1);}
    }
};
int main(){
    using namespace spark;
    Store store;CaptureApp<Store> app;
    app.open(store);store.finish(3);app.poll(store,100);
    assert(app.storageState().count==3);
    InputEvent e;e.text[0]='n';app.input(e,store);
    e={};strcpy(e.text,"ADV case list");app.input(e,store);
    e={};e.save=true;app.input(e,store);
    // Reproduce a successful save whose directory refresh only yields one item.
    strcpy(store.state.id,"0000000000000004");store.finish(1,false,true);
    app.poll(store,200);assert(app.storageState().count==3 && app.textFocused());
    assert(!app.leave(false,store));assert(store.lists==2);
    store.finish(0,false,false);app.poll(store,300);
    assert(app.storageState().count==3 && store.lists==3); // automatic retry, no reboot
    store.finish(4);app.poll(store,400);assert(app.storageState().count==4);
    store.finish(1,true,true);app.poll(store,420);
    assert(app.storageState().count==4); // A plausible-looking partial list needs a rescan too.
    store.finish(4);app.poll(store,440);
    // Busy or malformed published vectors cannot replace the complete list.
    store.state.busy=true;store.state.count=1;++store.state.revision;
    app.poll(store,450);assert(app.storageState().count==4);
    store.finish(2);strcpy(store.state.entries[1].id,store.state.entries[0].id);
    app.poll(store,500);assert(app.storageState().count==4 && !app.storageState().listValid);
    // Backoff prevents an SD retry loop. Hidden Cache never starts a rescan.
    auto count=store.lists;store.accept=false;app.poll(store,600);
    app.poll(store,700);assert(store.lists==count);
    app.poll(store,6000,false);assert(store.lists==count);
    store.accept=true;app.poll(store,6001);assert(store.lists==count+1);
    store.finish(4);app.poll(store,6100);
    // Empty is provisional until the storage worker confirms a fresh rescan.
    store.finish(0,true,true,false);app.poll(store,6200);
    assert(app.storageState().count==4);
    store.finish(0,true,true,true);app.poll(store,6300);
    assert(app.storageState().count==0 && app.storageState().listValid);
    app.draw();
    // The transactional scanner cannot mutate the live index before commit.
    NoteStorageState live;live.count=2;live.listValid=true;
    strcpy(live.entries[0].id,"0000000000000001");strcpy(live.entries[1].id,"0000000000000002");
    NoteIndexScan scan;scan.count=1;strcpy(scan.entries[0].id,"0000000000000003");
    assert(live.count==2 && validNoteIndex(live));
    scan.reset();assert(live.count==2); // aborted/failed scan
    scan.commit(live,true);assert(live.count==0 && live.emptyVerified);
}
