#pragma once
#include "State.h"
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdint>
inline uint32_t millis(){static auto start=std::chrono::steady_clock::now();return uint32_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count()*10);}
struct FakeSerial {
 int printf(const char* format,...){char bytes[512];va_list args;va_start(args,format);int n=vsnprintf(bytes,sizeof(bytes),format,args);va_end(args);std::lock_guard<std::mutex> lock(sshfake::logMutex());sshfake::logs().push_back(bytes);return n;}
};
static FakeSerial Serial;
struct FakeEsp {unsigned getFreeHeap(){return sshfake::counts().heap();}};
static FakeEsp ESP;
