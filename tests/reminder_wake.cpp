#include <cassert>
#include <cstring>
#include "../src/core/ReminderWake.h"
#include "../src/core/ScreenSleep.h"
struct Event {bool enter=false;char text[4]{};};
int main(){using namespace spark;
 ReminderWake wake;assert(!wake.brief() && !wake.expired(0));
 // Unattended: the screen sleeps again after the brief window.
 wake.woke(1000);assert(wake.brief() && !wake.expired(60999));
 assert(wake.expired(61000) && !wake.brief() && !wake.expired(70000));
 // Dismiss/snooze shortens the window so the result is seen, then sleeps.
 wake.woke(0);wake.reminderAction(5000);assert(!wake.expired(14999) && wake.expired(15000));
 // Other input hands control back to normal use.
 wake.woke(0);wake.userActivity();assert(!wake.expired(120000));
 wake.reminderAction(1);assert(!wake.brief());
 wake.woke(UINT32_MAX-1000);assert(!wake.expired(100) && wake.expired(60000));
 Event e;assert(reminderKey(e)==ReminderKey::None);
 e.enter=true;assert(reminderKey(e)==ReminderKey::Dismiss);
 e={};strcpy(e.text,"1");assert(reminderKey(e)==ReminderKey::Snooze10);
 e={};strcpy(e.text,"3");assert(reminderKey(e)==ReminderKey::Snooze30);
 e={};strcpy(e.text,"2");assert(reminderKey(e)==ReminderKey::None);
 // Only a sleeping screen is lit; no wake-gesture quarantine; returns to sleep.
 ScreenSleep screen;bool woke=false;screen.filter(false,false,false,false,woke);
 assert(!screen.reminderWake() && screen.reminderSleep() && screen.sleeping());
 assert(screen.reminderWake() && !screen.sleeping() && !screen.quarantined());
 assert(screen.reminderSleep() && screen.sleeping() && !screen.readyForStandby());
 screen.filter(false,false,false,false,woke);assert(screen.readyForStandby() && !woke);
}
