#pragma once
using gpio_num_t=int;
constexpr int GPIO_NUM_0=0,GPIO_NUM_11=11;
inline int gpio_get_level(int){return !fakePower::pending;}
