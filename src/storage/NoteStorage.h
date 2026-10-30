#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include "NoteData.h"
#include "NoteIndex.h"
#include "../terminal/TerminalData.h"
namespace spark {
class NoteStorage {
    struct Request { NoteOperation operation=NoteOperation::List; char id[17]{}, data[NoteLimit+1]{}; } request;
    NoteStorageState published,working;
    NoteIndexScan scan;
    bool mounted=false;
    SemaphoreHandle_t mutex=nullptr;
    QueueHandle_t queue=nullptr;
    bool inFlight=false;
    uint32_t operationStarted=0;
    char terminalCommand[TerminalCommandLimit+1]{},terminalCwd[128]="/";
    bool terminalConfirmed=false,terminalSystem=false;
    TerminalResult terminalPublished,terminalWorking;
    StorageStats stats;
    static void taskEntry(void* context);
    void run();
    void publish();
    bool mount(bool prepareNotes=true,bool force=false);
    bool list();
    bool scanDirectory();
    bool read();
    bool save();
    bool remove();
    bool reminder();
public:
    bool begin();
    bool submit(NoteOperation operation,const char* id="",const char* data="");
    bool snapshot(NoteStorageState& output);
    bool submitTerminal(const char* command,const char* cwd,bool confirmed=false,bool system=false);
    bool submitSystemTerminal(const char* command,const char* cwd){return submitTerminal(command,cwd,false,true);}
    bool terminalSnapshot(TerminalResult& output);
    bool requestStats();
    bool storageSnapshot(StorageStats& output);
    bool idle();
};
}
