#include <cassert>
#include <cstdlib>
#include "../src/core/CalendarMath.h"
// Calendar date math and the read-only reminder view: leap years, month lengths,
// weekdays, month/year transitions, the grid shape and Cache's repeat rules.
using namespace spark::calendar;
static Date D(int y,int m,int d){Date r;r.year=y;r.month=m;r.day=d;return r;}
int main() {
    setenv("TZ","UTC0",1); tzset();
    // Leap years and month lengths.
    assert(leapYear(2024) && !leapYear(2025) && leapYear(2028) && !leapYear(2100) && leapYear(2000));
    assert(daysInMonth(2024,2)==29 && daysInMonth(2025,2)==28 && daysInMonth(2026,4)==30 && daysInMonth(2026,12)==31);
    // Weekdays (0 = Sunday).
    assert(weekday(D(1970,1,1))==4 && weekday(D(2000,1,1))==6 && weekday(D(2024,1,1))==1);
    assert(weekday(D(2026,11,9))==1 && weekday(D(2026,12,25))==5 && weekday(D(2099,12,31))==4);
    // Every day of the supported range round-trips, with consecutive weekdays.
    long first=dayNumber(D(MinYear,1,1)),last=dayNumber(D(MaxYear,12,31));
    int previousWeekday=weekday(D(MinYear,1,1));
    for(long n=first;n<=last;++n){ Date d=fromDayNumber(n); assert(valid(d) && dayNumber(d)==n);
        if(n>first){ assert(weekday(d)==(previousWeekday+1)%7); } previousWeekday=weekday(d); }
    assert(last-first+1==76*365+19);   // 76 years 2024..2099, 19 leap years (2024, 2028 ... 2096).
    // Month and year transitions.
    assert(addDays(D(2026,12,31),1)==D(2027,1,1) && addDays(D(2027,1,1),-1)==D(2026,12,31));
    assert(addDays(D(2024,2,28),1)==D(2024,2,29) && addDays(D(2025,2,28),1)==D(2025,3,1));
    assert(addDays(D(2026,11,30),7)==D(2026,12,7) && addDays(D(2026,11,3),-7)==D(2026,10,27));
    assert(addMonths(D(2024,1,31),1)==D(2024,2,29) && addMonths(D(2025,1,31),1)==D(2025,2,28));
    assert(addMonths(D(2026,12,15),1)==D(2027,1,15) && addMonths(D(2026,1,15),-1)==D(2025,12,15));
    assert(addMonths(D(2026,3,31),-1)==D(2026,2,28) && addMonths(D(2026,5,31),1)==D(2026,6,30));
    // The supported range is clamped, never left.
    assert(addDays(D(2024,1,1),-1)==D(2024,1,1) && addDays(D(2099,12,31),1)==D(2099,12,31));
    assert(addMonths(D(2024,1,10),-1)==D(2024,1,1) && addMonths(D(2099,12,10),1)==D(2099,12,31));
    // Month grid: Sunday-first rows, 4 to 6 of them.
    MonthGrid g=monthGrid(2026,2); assert(g.offset==0 && g.days==28 && g.rows==4);   // Feb 1 2026 is a Sunday.
    g=monthGrid(2026,11); assert(g.offset==0 && g.days==30 && g.rows==5);
    g=monthGrid(2026,8); assert(g.offset==6 && g.days==31 && g.rows==6);              // Aug 1 2026 is a Saturday.
    for(int y=MinYear;y<=MaxYear;++y) for(int m=1;m<=12;++m){ g=monthGrid(y,m); assert(g.rows>=4 && g.rows<=6 && g.offset>=0 && g.offset<7); }
    // Reminders, read from Cache's schedule: next due date + repeat.
    ReminderRule once; once.next=D(2026,11,12); once.repeat=spark::ReminderRepeat::Once; once.active=true;
    assert(occursOn(once,D(2026,11,12)) && !occursOn(once,D(2026,11,13)) && !occursOn(once,D(2026,11,11)));
    ReminderRule daily=once; daily.repeat=spark::ReminderRepeat::Daily;
    assert(!occursOn(daily,D(2026,11,11)) && occursOn(daily,D(2026,11,12)) && occursOn(daily,D(2027,3,1)));
    ReminderRule weekly=once; weekly.repeat=spark::ReminderRepeat::Weekly;                  // Nov 12 2026 is a Thursday.
    assert(occursOn(weekly,D(2026,11,19)) && occursOn(weekly,D(2026,12,31)) && !occursOn(weekly,D(2026,11,20)) && !occursOn(weekly,D(2026,11,5)));
    ReminderRule weekdays=once; weekdays.repeat=spark::ReminderRepeat::Weekdays;
    assert(occursOn(weekdays,D(2026,11,13)) && !occursOn(weekdays,D(2026,11,14)) && !occursOn(weekdays,D(2026,11,15)) && occursOn(weekdays,D(2026,11,16)));
    assert(!occursOn(weekdays,D(2026,11,11)));
    ReminderRule off=daily; off.active=false; assert(!occursOn(off,D(2026,11,12)));
    // Navigation: open on today, arrows by day and week, months, today, Esc.
    CalendarView v; v.open(D(2026,11,9),true); assert(v.todayKnown && v.selected==D(2026,11,9) && !v.detail);
    v.move(-7); assert(v.selected==D(2026,11,2)); v.move(-7); assert(v.selected==D(2026,10,26));
    v.month(1); assert(v.selected==D(2026,11,26)); v.month(1); v.month(1); assert(v.selected==D(2027,1,26));
    assert(v.goToday() && v.selected==D(2026,11,9));
    v.detail=true; assert(v.back() && !v.detail); assert(!v.back());
    // Without a set clock: no "today", a fixed starting month, and T does nothing.
    CalendarView unknown; unknown.open(D(1970,1,1),false); assert(!unknown.todayKnown && valid(unknown.selected) && !unknown.goToday());
}
