#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
namespace spark {
constexpr unsigned TerminalColumns=38,TerminalLines=64,TerminalCommandLimit=160;
struct TerminalResult {
    char lines[TerminalLines][TerminalColumns+1]{};
    unsigned count=0; bool ok=true,busy=false,mutated=false; uint32_t revision=0,started=0;
    char cwd[128]="/";
    void line(const char* value) {
        if(count>=TerminalLines) return;
        snprintf(lines[count++],TerminalColumns+1,"%s",value);
    }
    void fail(const char* value) { ok=false; line(value); }
};
struct StorageStats { bool mounted=false,capacityValid=false; uint64_t total=0,used=0; uint32_t revision=0; };
}
