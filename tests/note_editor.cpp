#include <cassert>
#include <string>
#include "../src/core/NoteEditor.h"
int main() {
 spark::NoteEditor e; assert(e.insert('a')); assert(e.insert('b')); assert(!strcmp(e.data,"ab"));
 e.horizontal(-1); assert(e.insert('X')); assert(!strcmp(e.data,"aXb"));
 e.erase(); assert(!strcmp(e.data,"ab")); assert(e.dirty);
 assert(e.load("hello\r\nworld",12)); assert(!strcmp(e.data,"hello\nworld")); assert(!e.dirty);
 assert(!e.load("bad\tfile",8)); assert(!strcmp(e.data,"hello\nworld"));
 e.clear(); for(size_t i=0;i<spark::NoteLimit;++i) assert(e.insert('x'));
 assert(!e.insert('y')); assert(e.length==spark::NoteLimit && e.data[spark::NoteLimit]==0);
 assert(e.position(e.cursor).row>=e.firstRow && e.position(e.cursor).row<e.firstRow+spark::NoteVisibleRows);
 for(size_t i=0;i<spark::NoteLimit;++i) e.erase();
 assert(e.length==0 && !e.cursor);
 e.erase(); assert(!e.cursor);
 std::string wrapped(19,'x'); wrapped+="\nabc";
 assert(e.load(wrapped.c_str(),wrapped.size()));
 assert(e.position(19).row==1 && e.position(20).row==1);
 e.cursor=e.length; e.vertical(-1); assert(e.position(e.cursor).row==0);
 e.vertical(1); assert(e.position(e.cursor).row==1);
 e.clear(); e.insert('\n'); e.insert('\n'); assert(e.position(e.cursor).row==2);
 std::string ref; e.clear();
 uint32_t random=123;
 for(unsigned i=0;i<2000;++i) {
  random=random*1664525u+1013904223u;
  if(random%3==0) { if(e.cursor) ref.erase(e.cursor-1,1); e.erase(); }
  else if(random%3==1) { e.horizontal(-1); }
  else { ref.insert(e.cursor,1,'a'); assert(e.insert('a')); }
  assert(ref==e.data && ref.size()==e.length);
 }
}
