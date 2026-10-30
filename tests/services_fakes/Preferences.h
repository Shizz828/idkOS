#pragma once
#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <algorithm>
inline size_t strlcpy(char* dst,const char* src,size_t capacity) {
    size_t size=strlen(src); if(capacity) { size_t count=std::min(size,capacity-1); memcpy(dst,src,count); dst[count]=0; } return size;
}
class Preferences {
    std::string prefix;
public:
    static std::map<std::string,std::vector<uint8_t>>& data() { static std::map<std::string,std::vector<uint8_t>> entries; return entries; }
    static bool& writesFail() {static bool fail=false;return fail;}
    bool begin(const char* name,bool) { prefix=std::string(name)+"/"; return true; }
    void end() {}
    size_t getBytesLength(const char* key) { return data()[prefix+key].size(); }
    size_t getBytes(const char* key,void* target,size_t size) { auto& value=data()[prefix+key]; size=std::min(size,value.size()); if(size) memcpy(target,value.data(),size); return size; }
    size_t putBytes(const char* key,const void* source,size_t size) { if(writesFail())return 0;auto start=static_cast<const uint8_t*>(source); data()[prefix+key]=std::vector<uint8_t>(start,start+size); return size; }
};
