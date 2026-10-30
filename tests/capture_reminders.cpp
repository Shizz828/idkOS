#include <cassert>
#include <cstdlib>
#include "../src/apps/CaptureApp.h"
struct Store {
 spark::NoteStorageState state;bool accept=true;unsigned reminders=0;char options[40]{};
 bool submit(spark::NoteOperation op,const char* id="",const char* data=""){
  if(!accept)return false;
  state.operation=op;strcpy(state.id,id);state.busy=true;++state.revision;
  if(op==spark::NoteOperation::Reminder){++reminders;strcpy(options,data);}return true;
 }
 bool snapshot(spark::NoteStorageState& out){out=state;return true;}
 void finish(){state.busy=false;state.ok=state.listValid=true;++state.revision;}
};
int main(){
 setenv("TZ","EST5EDT,M3.2.0/2,M11.1.0/2",1);tzset();
 Store store;store.state.count=1;strcpy(store.state.entries[0].id,"0123456789abcdef");strcpy(store.state.entries[0].title,"Optional reminder");
 spark::CaptureApp<Store> app;app.open(store);store.finish();app.poll(store);app.draw();
 spark::InputEvent e;e.reminder=true;app.input(e,store);app.draw();
 auto key=[&](spark::CaptureApp<Store>& target,Store& storage,spark::InputEvent event){target.input(event,storage);target.draw();};
 auto enter=[]{spark::InputEvent v;v.enter=true;return v;};auto down=[]{spark::InputEvent v;v.down=true;return v;};
 auto up=[]{spark::InputEvent v;v.up=true;return v;};auto right=[]{spark::InputEvent v;v.right=true;return v;};
 auto save=[]{spark::InputEvent v;v.save=true;return v;};
 auto type=[](const char* digits){spark::InputEvent v;strcpy(v.text,digits);return v;};
 auto shows=[](const char* value){for(const auto& row:M5Cardputer.Display.printed)if(row.text==value)return true;return false;};
 // Fields are edited one at a time: Time, then Date, then Save.
 key(app,store,enter());key(app,store,type("1830"));key(app,store,enter());
 key(app,store,down());key(app,store,enter());key(app,store,type("10162099"));key(app,store,enter());
 assert(shows("18:30") && shows("10/16/2099"));
 key(app,store,down());key(app,store,down());key(app,store,down());key(app,store,enter());assert(store.reminders==0); // no trusted time
 spark::systemTimeSynchronized().store(true);
 key(app,store,up());key(app,store,up()); // Save -> Active -> Repeat
 for(int i=0;i<3;++i)key(app,store,right()); // Once -> Weekdays
 key(app,store,down());key(app,store,right());key(app,store,right()); // active off/on
 key(app,store,save());assert(store.reminders==1 && !strcmp(store.options,"2099-10-16 18:30|3|1"));
 store.state.entries[0].reminderAt=spark::parseReminderDate("2099-10-16 18:30");store.state.entries[0].repeat=spark::ReminderRepeat::Weekdays;
 store.finish();app.poll(store);e={};e.reminder=true;app.input(e,store);app.draw();
 assert(shows("18:30") && shows("10/16/2099") && shows("Weekdays") && shows("Yes")); // Existing values load.
 spark::systemTimeSynchronized().store(false);
 key(app,store,down());key(app,store,down());key(app,store,down());key(app,store,right()); // Active off only
 key(app,store,save());assert(store.reminders==2 && !strcmp(store.options,"2099-10-16 18:30|3|0"));
 store.finish();app.poll(store);e={};e.reminder=true;app.input(e,store);assert(!app.leave(false,store));app.draw(); // cancel leaves note intact
 spark::systemTimeSynchronized().store(true);
 // Edit only the time: 6:30 PM -> 8:00 PM in 12-hour mode; the date, repeat and active state stay.
 store.state.entries[0].repeat=spark::ReminderRepeat::Weekly;store.state.entries[0].reminderActive=true;
 store.finish();app.poll(store);spark::twelveHourClock()=true;
 e={};e.reminder=true;key(app,store,e);assert(shows("6:30 PM") && shows("10/16/2099"));
 key(app,store,enter());key(app,store,type("08"));key(app,store,type("00"));assert(shows("8:00 PM"));key(app,store,enter());
 key(app,store,save());assert(store.reminders==3 && !strcmp(store.options,"2099-10-16 20:00|2|1"));
 // Edit only the date: day 16 -> 20 with the time untouched.
 store.finish();app.poll(store);spark::twelveHourClock()=false;
 e={};e.reminder=true;key(app,store,e);
 key(app,store,down());key(app,store,enter());key(app,store,right());key(app,store,type("20"));key(app,store,enter());
 assert(shows("18:30") && shows("10/20/2099"));
 key(app,store,save());assert(store.reminders==4 && !strcmp(store.options,"2099-10-20 18:30|2|1"));
 // Cancel: Esc first undoes the field being edited, then closes the form; nothing is saved.
 store.finish();app.poll(store);
 e={};e.reminder=true;key(app,store,e);
 key(app,store,enter());key(app,store,type("0645"));assert(shows("06:45"));
 assert(!app.leave(false,store));app.draw();assert(app.textFocused() && shows("18:30") && shows("NOTE REMINDER"));
 assert(!app.leave(false,store));app.draw();assert(!shows("NOTE REMINDER") && store.reminders==4);
 assert(store.state.entries[0].reminderAt==spark::parseReminderDate("2099-10-16 18:30"));
 // Saving from an open, saved note must wait for SD completion and return to
 // the list, where the persisted reminder marker is visible.
 Store saved;spark::CaptureApp<Store> savedApp;saved.state.count=1;
 strcpy(saved.state.entries[0].id,"fedcba9876543210");strcpy(saved.state.entries[0].title,"Saved note");
 savedApp.open(saved);saved.finish();savedApp.poll(saved);
 e={};e.enter=true;savedApp.input(e,saved);strcpy(saved.state.data,"Saved note");saved.state.operation=spark::NoteOperation::Read;strcpy(saved.state.id,"fedcba9876543210");saved.finish();savedApp.poll(saved);assert(savedApp.textFocused());
 e={};e.reminder=true;savedApp.input(e,saved);
 key(savedApp,saved,enter());key(savedApp,saved,type("0730"));key(savedApp,saved,enter());
 key(savedApp,saved,down());key(savedApp,saved,enter());key(savedApp,saved,type("10162099"));key(savedApp,saved,enter());
 key(savedApp,saved,save());
 savedApp.draw();assert(shows("NOTE REMINDER")); // queue acceptance is not a save
 saved.state.busy=false;saved.state.ok=false;++saved.state.revision;strcpy(saved.state.message,"Reminder save failed");savedApp.poll(saved);savedApp.draw();assert(shows("NOTE REMINDER") && shows("07:30") && shows("10/16/2099"));
 key(savedApp,saved,save());assert(saved.reminders==2);
 saved.state.entries[0].reminderAt=spark::parseReminderDate("2099-10-16 07:30");saved.state.entries[0].reminderActive=true;
 saved.finish();savedApp.poll(saved);savedApp.draw();assert(!savedApp.textFocused());assert(shows("R") && shows("Saved note") && !shows("NOTE REMINDER"));
 assert(!strcmp(saved.options,"2099-10-16 07:30|0|1"));
 // Form parts: arrows adjust and wrap, days clamp to the month, invalid typing is ignored.
 {
  spark::ReminderForm form;form.load(spark::parseReminderDate("2028-02-29 23:59"),spark::ReminderRepeat::Once,true);
  form.input(enter());form.input(right());form.input(up());assert(form.hour==23 && form.minute==0); // minute wraps alone
  form.input(right());form.input(up());assert(form.hour==0); // 24-hour: no AM/PM part, wraps to hour
  form.input(type("99"));assert(form.hour==0);assert(form.back() && form.hour==23 && form.minute==59);
  form.input(down());form.input(enter());form.input(right());form.input(right());form.input(up()); // year 2029
  assert(form.year==2029 && form.month==2 && form.day==28); // Feb 29 clamps in a non-leap year
  form.input(enter());form.input(type("x"));assert(!form.editing && form.field==spark::ReminderForm::Date);
  spark::twelveHourClock()=true;form.input(up());form.input(enter());form.input(right());form.input(right());
  spark::InputEvent am;strcpy(am.text,"a");form.input(am);assert(form.hour==11); // 11:59 PM -> AM
  form.input(enter());spark::twelveHourClock()=false;
  char text[24];form.stored(text,sizeof(text));assert(!strcmp(text,"2029-02-28 11:59"));
  form.load(spark::parseReminderDate("2026-03-08 01:30"),spark::ReminderRepeat::Once,true);
  form.input(enter());form.input(type("02"));form.input(enter());assert(!form.due()); // DST gap rejected on save
 }
 // The reported example is valid and future relative to the report date.
 assert(spark::parseReminderDate("2026-10-16 07:30")>spark::parseReminderDate("2026-10-14 12:00"));

}
