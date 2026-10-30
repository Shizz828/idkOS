#pragma once
#include "FreeRTOS.h"
struct FakeTaskStop {};
inline TaskHandle_t& fakeWorker(){static TaskHandle_t task=nullptr;return task;}
inline TaskHandle_t xTaskCreateStaticPinnedToCore(void(*entry)(void*),const char*,unsigned bytes,void* context,unsigned,StackType_t* stack,StaticTask_t* storage,int){
 assert(bytes==32768 && stack);fakeWorker()=storage;sshfake::counts().tasks.fetch_add(1);
 storage->thread=std::thread([=]{currentTask()=storage;try{entry(context);}catch(const FakeTaskStop&){} });return storage;
}
inline unsigned ulTaskNotifyTake(int,unsigned ticks){
 auto task=currentTask();std::unique_lock<std::mutex> lock(task->mutex);
 task->cv.wait_for(lock,std::chrono::milliseconds(ticks/10),[&]{return task->stop || task->notifications;});
 if(task->stop)throw FakeTaskStop{};
 unsigned n=task->notifications;task->notifications=0;return n;
}
inline void xTaskNotifyGive(TaskHandle_t task){std::lock_guard<std::mutex> lock(task->mutex);++task->notifications;task->cv.notify_all();}
inline void vTaskDelay(unsigned){std::this_thread::sleep_for(std::chrono::milliseconds(1));}
inline unsigned uxTaskGetStackHighWaterMark(TaskHandle_t){return sshfake::counts().stackMargin.load();}
