#pragma once
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include <initializer_list>
#include "StandbyPolicy.h"
namespace spark {
constexpr gpio_num_t StandbyHomePin=GPIO_NUM_0;
constexpr gpio_num_t StandbyKeyboardPin=GPIO_NUM_11; // ADV TCA8418 IRQ, active low.
constexpr uint64_t StandbyHomeMask=1ULL<<StandbyHomePin;
constexpr uint64_t StandbyKeyboardMask=1ULL<<StandbyKeyboardPin;
struct StandbyWake { esp_err_t error=ESP_OK;uint64_t keys=0;bool entered=false; };
inline bool standbyInputPending() {
    return !gpio_get_level(StandbyHomePin) || !gpio_get_level(StandbyKeyboardPin);
}
inline StandbyWake enterLightSleep(uint64_t intervalUs=StandbyIntervalUs) {
    StandbyWake result;
    // EXT1 leaves the keyboard pin's digital setup alone.
    // Both ADV wake pins are RTC-capable on ESP32-S3.
    result.error=esp_sleep_enable_timer_wakeup(intervalUs);
    if(result.error==ESP_OK)result.error=esp_sleep_enable_ext1_wakeup(
        StandbyHomeMask|StandbyKeyboardMask,ESP_EXT1_WAKEUP_ANY_LOW);
    if(result.error==ESP_OK)result.error=esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH,ESP_PD_OPTION_ON);
    for(auto pin:{StandbyHomePin,StandbyKeyboardPin}) {
        rtc_gpio_pullup_en(pin);rtc_gpio_pulldown_dis(pin);
    }
    if(result.error==ESP_OK && !standbyInputPending()) {
        result.entered=true;
        result.error=esp_light_sleep_start();
        if(result.error==ESP_OK && esp_sleep_get_wakeup_cause()==ESP_SLEEP_WAKEUP_EXT1)
            result.keys=esp_sleep_get_ext1_wakeup_status();
    }
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT1);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
    for(auto pin:{StandbyHomePin,StandbyKeyboardPin}) {
        rtc_gpio_hold_dis(pin);rtc_gpio_deinit(pin);
    }
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH,ESP_PD_OPTION_AUTO);
    return result;
}
}
