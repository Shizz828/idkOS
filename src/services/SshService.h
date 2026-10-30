#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <atomic>
#include "../ssh/SshData.h"
namespace spark {
class SshService {
    static constexpr uint32_t WorkerStackBytes=32768;
    StaticSemaphore_t mutexStorage;
    SemaphoreHandle_t mutex=nullptr;
    StaticTask_t workerControl;
    alignas(16) StackType_t workerStack[WorkerStackBytes/sizeof(StackType_t)];
    TaskHandle_t worker=nullptr;
    std::atomic<bool> running{false},cancelled{false};
    std::atomic<int> decision{0},scrollBy{0};
    SshSnapshot published;
    SshProfile requested;
    char password[129]{};
    SshInputBuffer outbound;
    char launchError[81]{};
    std::atomic<unsigned> attempt{0};
    static void taskEntry(void* context);
    void run();
    void publish(const SshSnapshot& state);
    void logHeap(const char* stage)const;
public:
    bool begin();
    bool connect(const SshProfile& profile,const char* secret);
    const char* startError()const{return launchError;}
    bool busy()const{return running.load();}
    bool snapshot(SshSnapshot& state);
    bool send(const char* bytes,unsigned size);
    void disconnect(){cancelled.store(true);}
    void verify(int choice){decision.store(choice);}
    void scroll(int amount){scrollBy.fetch_add(amount);}
};
}
