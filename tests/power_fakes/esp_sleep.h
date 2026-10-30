#pragma once
#include <cstdint>
#include <initializer_list>
using esp_err_t=int;
constexpr int ESP_OK=0,ESP_SLEEP_WAKEUP_EXT1=3,ESP_SLEEP_WAKEUP_TIMER=4;
constexpr int ESP_EXT1_WAKEUP_ANY_LOW=0,ESP_PD_DOMAIN_RTC_PERIPH=0,ESP_PD_OPTION_ON=1,ESP_PD_OPTION_AUTO=2;
namespace fakePower {
static int error=0,cause=ESP_SLEEP_WAKEUP_TIMER,entries=0,cleanups=0,rtcRestores=0;
static uint64_t interval=0,mask=0,wake=0;static bool pending=false;
}
inline int esp_sleep_enable_timer_wakeup(uint64_t us){fakePower::interval=us;return fakePower::error;}
inline int esp_sleep_enable_ext1_wakeup(uint64_t mask,int mode){fakePower::mask=mask;return mode;}
inline int esp_sleep_pd_config(int,int){return 0;}
inline int esp_light_sleep_start(){++fakePower::entries;return fakePower::error;}
inline int esp_sleep_get_wakeup_cause(){return fakePower::cause;}
inline uint64_t esp_sleep_get_ext1_wakeup_status(){return fakePower::wake;}
inline int esp_sleep_disable_wakeup_source(int){++fakePower::cleanups;return 0;}
