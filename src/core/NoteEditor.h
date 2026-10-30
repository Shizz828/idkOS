#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
namespace spark {
constexpr size_t NoteLimit=4096;
constexpr unsigned NoteColumns=19, NoteVisibleRows=5;
struct NotePosition { unsigned row=0,column=0; };
inline void advanceNote(NotePosition& p,char c,char previous,size_t index) {
    if(c=='\n') { if(p.column || index==0 || previous=='\n') ++p.row; p.column=0; }
    else if(++p.column>=NoteColumns) { ++p.row; p.column=0; }
}
class NoteEditor {
public:
    char data[NoteLimit+1]{};
    size_t length=0,cursor=0;
    unsigned firstRow=0;
    bool dirty=false;
    uint32_t generation=0;
    void clear() { data[0]=0; length=cursor=firstRow=0; dirty=false; ++generation; }
    bool load(const char* source,size_t size) {
        if(size>NoteLimit) return false;
        for(size_t i=0;i<size;++i)
            if(source[i]!='\n' && source[i]!='\r' && (uint8_t(source[i])<32 || uint8_t(source[i])>126)) return false;
        clear();
        for(size_t i=0;i<size;++i) if(source[i]!='\r') data[length++]=source[i];
        data[length]=0; return true;
    }
    bool insert(char c) {
        if(length==NoteLimit) return false;
        memmove(data+cursor+1,data+cursor,length-cursor+1);
        data[cursor++]=c; ++length; dirty=true; ++generation; reveal(); return true;
    }
    void erase() {
        if(!cursor) return;
        memmove(data+cursor-1,data+cursor,length-cursor+1);
        --cursor; --length; dirty=true; ++generation; reveal();
    }
    NotePosition position(size_t index) const {
        NotePosition p;
        for(size_t i=0;i<index && i<length;++i) advanceNote(p,data[i],i?data[i-1]:0,i);
        return p;
    }
    void reveal() {
        auto p=position(cursor);
        if(p.row<firstRow) firstRow=p.row;
        if(p.row>=firstRow+NoteVisibleRows) firstRow=p.row-NoteVisibleRows+1;
    }
    void horizontal(int delta) {
        if(delta<0 && cursor) --cursor;
        if(delta>0 && cursor<length) ++cursor;
        reveal();
    }
    void vertical(int delta) {
        auto current=position(cursor);
        unsigned target=delta<0 ? (current.row?current.row-1:0) : current.row+1;
        NotePosition p; size_t best=cursor; bool found=false;
        for(size_t i=0;i<=length;++i) {
            if(p.row==target && p.column<=current.column) { best=i; found=true; }
            if(p.row>target) break;
            if(i<length) advanceNote(p,data[i],i?data[i-1]:0,i);
        }
        if(found) cursor=best;
        reveal();
    }
};
}
