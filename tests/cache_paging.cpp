#include <cassert>
#include <cstring>
#include <string>
#include "../src/apps/CaptureApp.h"
// Saving a note selects it. When it lands beyond the first three rows, the
// earlier notes are on another page and must be signposted, not appear lost.
struct Store {
 spark::NoteStorageState result;
 bool submit(spark::NoteOperation op,const char* ="",const char* =""){result.operation=op;result.busy=true;++result.revision;return true;}
 bool snapshot(spark::NoteStorageState& out){out=result;return true;}
 void entries(unsigned count){
  result.count=count;
  for(unsigned i=0;i<count;++i){snprintf(result.entries[i].id,17,"%016x",i+1);snprintf(result.entries[i].title,33,"Note %u",i+1);}
 }
 void finish(){result.busy=false;result.ok=true;result.listValid=true;result.emptyVerified=!result.count;++result.revision;}
};
static bool shown(const char* text){
 for(const auto& p:M5Cardputer.Display.printed)if(p.text==text)return true;
 return false;
}
int main(){
 Store store;spark::CaptureApp<Store> app;
 store.entries(4);app.open(store);store.finish();app.poll(store);
 app.draw();assert(shown("Note 1") && shown("1-3 of 4"));
 // Create and save a fifth note; directory order places it last.
 spark::InputEvent e;strcpy(e.text,"n");app.input(e,store);
 e={};strcpy(e.text,"Shopping list");app.input(e,store);
 e={};e.save=true;app.input(e,store);
 store.entries(5);strcpy(store.result.id,store.result.entries[4].id);store.finish();app.poll(store);
 assert(!app.leave(false,store));store.finish();app.poll(store);
 app.draw();
 assert(!app.textFocused() && shown("Note 5") && !shown("Note 1"));
 assert(shown("4-5 of 5")); // Earlier notes are still reachable on page one.
 e={};e.up=true;app.input(e,store);e={};e.up=true;app.input(e,store);
 app.draw();assert(shown("Note 1") && shown("Note 3") && shown("1-3 of 5"));
 // A single page needs no indicator.
 Store small;spark::CaptureApp<Store> single;small.entries(3);single.open(small);small.finish();single.poll(small);
 single.draw();assert(shown("Note 3") && !shown("1-3 of 3"));
}
