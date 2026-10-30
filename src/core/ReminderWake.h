#pragma once
#include <stdint.h>
namespace spark {
// A reminder that becomes due while the screen sleeps lights it briefly;
// without other input the screen sleeps again.
class ReminderWake {
    bool active=false;
    uint32_t deadline=0;
public:
    static constexpr uint32_t WakeMs=60000,AfterActionMs=10000;
    void woke(uint32_t now){active=true;deadline=now+WakeMs;}
    // Dismiss/snooze from a brief wake: leave the result visible, then sleep.
    void reminderAction(uint32_t now){if(active)deadline=now+AfterActionMs;}
    // Any other key returns to the normal idle policy.
    void userActivity(){active=false;}
    bool brief()const{return active;}
    bool expired(uint32_t now) {
        if(!active || int32_t(now-deadline)<0)return false;
        active=false;return true;
    }
};
enum class ReminderKey : uint8_t { None, Dismiss, Snooze10, Snooze30 };
template<class Event> ReminderKey reminderKey(const Event& e) {
    if(e.enter)return ReminderKey::Dismiss;
    if(e.text[0]=='1')return ReminderKey::Snooze10;
    if(e.text[0]=='3')return ReminderKey::Snooze30;
    return ReminderKey::None;
}
}
