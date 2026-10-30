#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
namespace spark {
template<typename Filesystem>
bool recoverNote(Filesystem& fs,const char* path,const char* backup) {
    return fs.exists(path) || !fs.exists(backup) || fs.rename(backup,path);
}
template<typename Filesystem>
bool commitNote(Filesystem& fs,const char* path,const char* temporary,const char* backup,const char* data,size_t length) {
    auto file=fs.open(temporary,"w");
    if(!file) return false;
    bool ok=file.write(reinterpret_cast<const uint8_t*>(data),length)==length;
    file.flush(); file.close();
    if(!ok) return false;
    auto check=fs.open(temporary,"r");
    if(!check || check.size()!=length) { if(check) check.close(); return false; }
    uint8_t buffer[128]; size_t offset=0;
    while(offset<length) {
        size_t count=length-offset; if(count>sizeof(buffer)) count=sizeof(buffer);
        if(check.read(buffer,count)!=int(count) || memcmp(buffer,data+offset,count)) { check.close(); return false; }
        offset+=count;
    }
    check.close();
    if(fs.exists(backup)) {
        if(!fs.exists(path)) { if(!fs.rename(backup,path)) return false; }
        else if(!fs.remove(backup)) return false;
    }
    bool replacing=fs.exists(path);
    if(replacing && !fs.rename(path,backup)) return false;
    if(!fs.rename(temporary,path)) {
        if(replacing && !fs.exists(path)) fs.rename(backup,path);
        return false;
    }
    if(replacing) fs.remove(backup); // A leftover old copy is safe and recoverable.
    return true;
}
}
