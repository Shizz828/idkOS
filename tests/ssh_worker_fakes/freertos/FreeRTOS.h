#pragma once
#include <stdint.h>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <chrono>
#include "../State.h"
using StackType_t=uint8_t;
using TickType_t=uint32_t;
constexpr unsigned portMAX_DELAY=UINT32_MAX;
constexpr int pdTRUE=1,pdFALSE=0;
#define pdMS_TO_TICKS(ms) (ms)
struct StaticTask_t {
 std::mutex mutex;std::condition_variable cv;std::thread thread;
 bool stop=false;unsigned notifications=0;
};
using TaskHandle_t=StaticTask_t*;
inline TaskHandle_t& currentTask(){static thread_local TaskHandle_t task=nullptr;return task;}
