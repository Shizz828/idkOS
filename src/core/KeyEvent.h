#pragma once
#include <stdint.h>
#include <algorithm>
#include <vector>
namespace spark {
// One key event from the keyboard chip (TCA8418) onto the held-key list.
// The chip holds IRQ low until its events are read and acknowledged, so reading
// follows that level. An interrupt edge remembered in a flag can be lost, and the
// chip sends no second edge: the keyboard then stops with its held keys stuck.
constexpr uint8_t KeyInterruptStatus=0x02,KeyEventInterrupt=0x01;
template<typename Chip,typename Point>
inline void readKeyEvent(Chip& chip,bool irqLow,std::vector<Point>& keys) {
    if(!irqLow)return;
    const uint8_t event=chip.getEvent(),code=event&0x7F;
    chip.writeRegister8(KeyInterruptStatus,KeyEventInterrupt); // Stays set while events remain.
    if(!code || code>80)return; // Nothing queued, or not a key of the matrix.
    const unsigned row=(code-1)/10,col=(code-1)%10;
    Point key;key.x=int(row*2+(col>3?1:0));key.y=int(col%4); // The same layout as the library's reader.
    const auto found=std::find(keys.begin(),keys.end(),key);
    if(event&0x80){if(found==keys.end())keys.push_back(key);}
    else if(found!=keys.end())keys.erase(found);
}
}
