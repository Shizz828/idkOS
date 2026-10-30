#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include "CryptoData.h"
namespace spark {
class CryptoService {
    struct KeyCommand { char key[128]{}; };
    SemaphoreHandle_t mutex=nullptr;
    QueueHandle_t keys=nullptr;
    CryptoSnapshot published, working;
    CryptoSchedule schedule;
    char apiKey[128]{};
    bool loaded=false;
    void publish();
    void load();
    bool saveCache();
public:
    bool begin();
    bool snapshot(CryptoSnapshot& output);
    bool configureKey(const char* key);
    void poll(bool online,int64_t now); // Only called by the shared network worker.
};
}
