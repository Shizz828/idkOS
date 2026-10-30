#include <cassert>
#include <string>
#include "../src/storage/NoteData.h"
struct Reader {
 std::string content; unsigned cursor=0;
 bool available() { return cursor<content.size(); }
 int read(){return static_cast<unsigned char>(content[cursor++]);}
};
int main() {
 char title[33]; Reader malformed; malformed.content=std::string(100000, '\1');
 assert(spark::readNoteTitle(malformed,title,sizeof(title))==0); assert(malformed.cursor==256);
 Reader normal; normal.content="First line\nSecond line";
 assert(spark::readNoteTitle(normal,title,sizeof(title))==10 && !strcmp(title,"First line"));
 Reader empty; assert(!spark::readNoteTitle(empty,title,sizeof(title)));
}
