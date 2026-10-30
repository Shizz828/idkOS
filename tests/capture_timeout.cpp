#include <cassert>
#include "../src/apps/CaptureApp.h"
struct Store {
 spark::NoteStorageState state; char saved[spark::NoteLimit+1]{};
 bool submit(spark::NoteOperation op,const char* ="",const char* data="") {state.busy=true; state.started=10; state.operation=op; ++state.revision; strcpy(saved,data); return true;}
 bool snapshot(spark::NoteStorageState& out){out=state; return true;}
 void done(){state.busy=false;state.ok=state.listValid=true;state.emptyVerified=!state.count;++state.revision;strcpy(state.id,"0123456789abcdef");}
};
int main(){
 Store store; spark::CaptureApp<Store> app; spark::InputEvent e;
 e.text[0]='n'; app.input(e,store); e={}; strcpy(e.text,"old"); app.input(e,store);
 e={}; e.save=true; app.input(e,store); app.poll(store,100);
 assert(app.poll(store,15010)); e={}; e.text[0]='X'; app.input(e,store); app.draw();
 store.done(); app.poll(store,15020); app.draw();
 e={}; e.save=true; app.input(e,store); assert(!strcmp(store.saved,"oldX"));
}
