#include <cassert>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include "../src/apps/CalendarApp.h"
// Calendar screens on the 240x135 panel (the display fake asserts
// bounds): 4-, 5- and 6-row months, today and the selection anywhere, reminder
// marks, the day view with none, a few and more reminders than fit.
using namespace spark;
static ScheduledReminder reminder(const char* id,const char* title,int64_t due,ReminderRepeat repeat,int hour,int minute,bool active=true){
    ScheduledReminder r; strncpy(r.id,id,sizeof r.id-1); strncpy(r.text,title,sizeof r.text-1);
    r.due=due; r.repeat=repeat; r.hour=hour; r.minute=minute; r.active=active; return r;
}
int main() {
    setenv("TZ","UTC0",1); tzset();
    const int64_t nov9=1794182400;   // 2026-11-09 00:00 UTC, a Monday.
    ScheduledReminder items[MaxNotes];
    items[0]=reminder("a","Take out the recycling bins",nov9+3*86400+19*3600,ReminderRepeat::Weekly,19,0);
    items[1]=reminder("b","Standup",nov9+9*3600+30*60,ReminderRepeat::Weekdays,9,30);
    items[2]=reminder("c","Water plants",nov9+8*3600,ReminderRepeat::Daily,8,0);
    items[3]=reminder("d","Dentist",nov9+86400*5+14*3600,ReminderRepeat::Once,14,0);
    items[4]=reminder("e","Old one",nov9-86400,ReminderRepeat::Once,9,0,false);
    CalendarApp app; app.open(nov9+12*3600,true); app.prepare(items,MaxNotes,nov9+12*3600,true);
    assert(app.count==4 && app.view.todayKnown);
    // Today: Water plants 8:00 then Standup 9:30, in time order.
    unsigned list[MaxNotes]; assert(app.dayList(list,MaxNotes)==2 && !strcmp(app.entries[list[0]].title,"Water plants"));
    assert(app.remindersOn(app.view.selected)==2);
    calendar::Date sat; sat.year=2026; sat.month=11; sat.day=14; assert(app.remindersOn(sat)==2);   // Daily + Dentist.
    // Enter opens the day; Esc closes it first.
    CalendarKeys enter; enter.enter=true; assert(app.input(enter) && app.view.detail);
    assert(app.back() && !app.view.detail && !app.back());
    // Many reminders on one day: the day view scrolls.
    ScheduledReminder busy[MaxNotes];
    for(unsigned i=0;i<12;++i){ char id[8]; snprintf(id,sizeof id,"r%u",i);
        busy[i]=reminder(id,"A very long reminder title that does not fit",nov9+int64_t(i)*3600,ReminderRepeat::Daily,int(i),5); }
    CalendarApp many; many.open(nov9,true); many.prepare(busy,MaxNotes,nov9,true); many.input(enter);
    CalendarKeys down; down.down=true;
    for(int i=0;i<12;++i) many.input(down);
    assert(many.view.detailTop==12-CalendarApp::DetailRows);
    {   // One Calendar look.
        for(bool twelve:{false,true}){ twelveHourClock()=twelve;
            many.view.detailTop=0; many.draw(); many.view.detailTop=12-CalendarApp::DetailRows; many.draw(); }
        // Every day of a 4-, 5- and 6-row month selected, plus months across years.
        for(int y:{2026,2027}) for(int m:{2,8,11,12}){ for(int d=1;d<=calendar::daysInMonth(y,m);++d){
            app.view.selected.year=y; app.view.selected.month=m; app.view.selected.day=d; app.draw(); } }
        app.view.detail=true; app.draw(); app.view.detail=false;
        CalendarApp none; none.open(0,false); none.prepare(items,MaxNotes,0,false); none.draw(); none.view.detail=true; none.draw();
    }
}
