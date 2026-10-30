#include <cassert>
#include <map>
#include <string>
#include <algorithm>
#include "../src/storage/NoteTransaction.h"
#include "../src/storage/NoteReminderStorage.h"
#include "../src/core/ReminderEngine.h"
struct Fs {
 std::map<std::string,std::string> files;
 bool shortWrite=false,failCommit=false,interrupt=false,failReminderRead=false;
 struct File {
  Fs* fs; std::string path; size_t offset=0; bool valid=true;
  explicit operator bool() const {return valid;}
  size_t write(const uint8_t* data,size_t size) {
   size_t n=fs->shortWrite && size ? size-1 : size;
   fs->files[path].assign(reinterpret_cast<const char*>(data),n); return n;
  }
  bool isDirectory() const{return false;}
  size_t size() const {return fs->files[path].size();}
  int read(uint8_t* out,size_t size) {
   size_t n=std::min(size,fs->files[path].size()-offset);
   memcpy(out,fs->files[path].data()+offset,n); offset+=n; return int(n);
  }
  void flush() {} void close() {}
 };
 File open(const char* p,const char* mode) {
  bool valid=*mode=='w' || files.count(p);
  if(*mode=='r' && failReminderRead && std::string(p)=="meta")valid=false;
  if(*mode=='w') files[p]="";
  File file; file.fs=this; file.path=p; file.offset=0; file.valid=valid; return file;
 }
 bool exists(const char* path){return files.count(path);}
 bool remove(const char* path){return files.erase(path);}
 bool rename(const char* from,const char* to){
  if(std::string(from)=="tmp" && interrupt) throw 1;
  if(std::string(from)=="tmp" && failCommit) return false;
  if(!files.count(from) || files.count(to)) return false;
  files[to]=files[from]; files.erase(from); return true;
 }
};
int main() {
 Fs fs; fs.files["note"]="old";
 assert(spark::commitNote(fs,"note","tmp","bak","new",3)); assert(fs.files["note"]=="new" && !fs.exists("bak"));
 fs.shortWrite=true;
 assert(!spark::commitNote(fs,"note","tmp","bak","broken",6)); assert(fs.files["note"]=="new");
 fs.shortWrite=false; fs.failCommit=true;
 assert(!spark::commitNote(fs,"note","tmp","bak","other",5)); assert(fs.files["note"]=="new");
 fs.failCommit=false; fs.interrupt=true;
 try {spark::commitNote(fs,"note","tmp","bak","other",5); assert(false);} catch(int) {}
 assert(!fs.exists("note") && fs.files["bak"]=="new");
 assert(spark::recoverNote(fs,"note","bak")); assert(fs.files["note"]=="new");
 fs.interrupt=false; assert(spark::commitNote(fs,"note","tmp","bak","",0)); assert(fs.files["note"].empty());
 spark::NoteReminder reminder;reminder.due=1792193400;reminder.repeat=spark::ReminderRepeat::Daily;reminder.hour=18;reminder.minute=30;
 assert(spark::saveNoteReminder(fs,"meta","tmp","bak",reminder));spark::NoteReminder loaded;assert(spark::loadNoteReminder(fs,"meta",loaded));assert(spark::sameReminder(reminder,loaded));
 auto next=reminder;next.due+=86400;assert(spark::advanceNoteReminder(fs,"note","meta","tmp","bak",reminder,next));assert(spark::loadNoteReminder(fs,"meta",loaded)&&loaded.due==next.due);
 auto user=next;user.active=false;assert(spark::saveNoteReminder(fs,"meta","tmp","bak",user));assert(spark::advanceNoteReminder(fs,"note","meta","tmp","bak",reminder,next));assert(spark::loadNoteReminder(fs,"meta",loaded)&&!loaded.active); // user edit wins
 fs.shortWrite=true;assert(!spark::saveNoteReminder(fs,"meta","tmp","bak",next));fs.shortWrite=false;assert(spark::loadNoteReminder(fs,"meta",loaded)&&!loaded.active);
 fs.files.erase("meta");assert(spark::advanceNoteReminder(fs,"note","meta","tmp","bak",reminder,next));assert(!fs.exists("meta")); // no resurrection
 fs.files["bak"]="1792193400";assert(spark::advanceNoteReminder(fs,"note","meta","tmp","bak",reminder,next));assert(spark::loadNoteReminder(fs,"meta",loaded));
 fs.files.erase("note");fs.files.erase("meta");assert(spark::advanceNoteReminder(fs,"note","meta","tmp","bak",reminder,next));assert(!fs.exists("meta"));

 // The reported date round-trips through the actual transactional sidecar codec.
 spark::NoteReminder reported;reported.due=spark::parseReminderDate("2026-10-16 07:30");assert(reported.due);
 assert(spark::saveNoteReminder(fs,"meta","tmp","bak",reported));assert(spark::loadNoteReminder(fs,"meta",loaded));assert(spark::sameReminder(reported,loaded));
 // A successful rename alone is insufficient if the final metadata cannot be
 // reloaded for Capture's R indicator. Retrying the same save is safe.
 fs.failReminderRead=true;assert(!spark::saveNoteReminder(fs,"meta","tmp","bak",reported));fs.failReminderRead=false;
 assert(spark::saveNoteReminder(fs,"meta","tmp","bak",reported));assert(spark::loadNoteReminder(fs,"meta",loaded)&&loaded.active);

}
