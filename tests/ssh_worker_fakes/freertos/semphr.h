#pragma once
#include "FreeRTOS.h"
struct StaticSemaphore_t {std::timed_mutex mutex;};
using SemaphoreHandle_t=StaticSemaphore_t*;
inline SemaphoreHandle_t xSemaphoreCreateMutexStatic(StaticSemaphore_t* storage){return storage;}
inline int xSemaphoreTake(SemaphoreHandle_t handle,unsigned ticks){if(ticks==portMAX_DELAY){handle->mutex.lock();return pdTRUE;}return handle->mutex.try_lock_for(std::chrono::milliseconds(ticks))?pdTRUE:pdFALSE;}
inline void xSemaphoreGive(SemaphoreHandle_t handle){handle->mutex.unlock();}
