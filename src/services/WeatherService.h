#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include "WeatherData.h"
#include "../core/NetworkStandby.h"
namespace spark {
class CryptoService;
class WeatherService {
    CryptoService* market=nullptr;
    struct Credentials { uint8_t networkAction=0,networkIndex=0; bool locationCommand=false,saveHome=false; LocationMode mode=LocationMode::Auto; WeatherLocation location; uint32_t attempt=0; char ssid[33]{}; char password[65]{}; };
    uint32_t nextAttempt=0;
    SemaphoreHandle_t mutex=nullptr;
    QueueHandle_t commands=nullptr;
    WeatherSnapshot published;
    NetworkStandby standby;
    static void taskEntry(void* context);
    void run();
    void publish(WeatherSnapshot& state);
public:
    bool begin(CryptoService* crypto=nullptr);
    void setStandby(bool value){standby.request(value);}
    bool standbyReady()const{return !mutex || standby.ready();}
    bool snapshot(WeatherSnapshot& out);
    bool configureLocation(LocationMode mode,const WeatherLocation& location,bool saveHome=false);
    uint32_t connect(const char* ssid, const char* password);
    // 1: connect saved, 2: forget saved, 3: reconnect, 4: scan nearby.
    bool networkAction(uint8_t action,unsigned index=0);
};
}
