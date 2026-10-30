#include <cassert>
#include <cstring>
#include "../src/apps/CaptureApp.h"
struct Store {
 spark::NoteStorageState result;
 bool accept=true; unsigned saves=0,deletes=0,reads=0; char saved[spark::NoteLimit+1]{};
 bool submit(spark::NoteOperation op,const char* id="",const char* data="") {
  if(!accept) return false;
  result.operation=op; result.busy=true; ++result.revision;
  if(op==spark::NoteOperation::Save) { ++saves; strcpy(saved,data); }
  if(op==spark::NoteOperation::Delete) ++deletes;
  if(op==spark::NoteOperation::Read) ++reads;
  (void)id; return true;
 }
 bool snapshot(spark::NoteStorageState& out){out=result; return true;}
 void finish(bool ok) {result.busy=false; result.ok=ok; result.listValid=ok; result.emptyVerified=ok && !result.count; ++result.revision; strcpy(result.message,ok?"Saved to microSD":"Save failed; draft kept in RAM");}
};
int main() {
 Store store; spark::CaptureApp<Store> app;
 app.open(store); app.poll(store); app.draw(); store.finish(true); app.poll(store);
 spark::InputEvent e; strcpy(e.text,"n"); app.input(e,store); app.draw(); assert(app.textFocused());
 e={}; strcpy(e.text,"Tomorrow note"); app.input(e,store); app.draw();
 assert(app.leave(true,store)); // Home preserves the draft.
 app.open(store); assert(app.textFocused());
 e={}; e.save=true; app.input(e,store); assert(store.saves==1 && !strcmp(store.saved,"Tomorrow note"));
 e={}; strcpy(e.text,"X"); app.input(e,store); // No edit while save request is in flight.
 store.finish(false); app.poll(store); app.draw(); assert(app.textFocused());
 e={}; e.save=true; app.input(e,store); assert(store.saves==2 && !strcmp(store.saved,"Tomorrow note"));
 strcpy(store.result.id,"0123456789abcdef"); store.finish(true); app.poll(store); app.draw();
 e={}; e.enter=true; app.input(e,store); e={}; strcpy(e.text,"Second line"); app.input(e,store); app.draw();
 assert(!app.leave(false,store)); app.draw(); // Unsaved Back needs confirmation.
 assert(!app.leave(false,store)); // Back cancels the confirmation.
 e={}; e.erase=true; app.input(e,store); app.draw();
 for(int i=0;i<200;++i) {e={}; strcpy(e.text,"abcdefghij"); app.input(e,store);}
 app.draw(); // Scrolled editor remains in bounds.
 assert(!app.leave(false,store)); e={}; e.enter=true; app.input(e,store);
 store.result.count=1; strcpy(store.result.entries[0].id,"0123456789abcdef"); strcpy(store.result.entries[0].title,"A note to delete");
 store.finish(true); app.poll(store); app.draw(); assert(!app.textFocused());
 e={}; e.erase=e.enter=e.enterDown=true; app.input(e,store); app.draw(); assert(store.deletes==0 && store.reads==0);
 e={}; e.enterDown=true; app.input(e,store); assert(store.deletes==0);
 e={}; app.input(e,store);
 e={}; e.enter=true; app.input(e,store); assert(store.deletes==1);
}
