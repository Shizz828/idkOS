#pragma once
inline int rtc_gpio_pullup_en(int){return 0;}
inline int rtc_gpio_pulldown_dis(int){return 0;}
inline int rtc_gpio_hold_dis(int){return 0;}
inline int rtc_gpio_deinit(int){++fakePower::rtcRestores;return 0;}
