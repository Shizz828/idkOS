#pragma once
#include <stdint.h>
#include "NoteReminder.h"
namespace spark {
// Calendar: plain civil-date math (no time zones, no allocation) and the read-only
// view of Cache's reminders. Nothing here stores or changes a reminder.
namespace calendar {
constexpr int MinYear=2024,MaxYear=2099;   // The reminder clock's range (see ReminderEngine).
struct Date { int year=2026,month=1,day=1; };
inline bool operator==(const Date& a,const Date& b){return a.year==b.year && a.month==b.month && a.day==b.day;}
inline bool operator!=(const Date& a,const Date& b){return !(a==b);}
inline bool leapYear(int y){return (y%4==0 && y%100!=0) || y%400==0;}
inline int daysInMonth(int y,int m){
    static const int days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    return (m==2 && leapYear(y))?29:days[m-1];
}
// Days since 1970-01-01 (H. Hinnant's days_from_civil); negative before 1970.
inline long dayNumber(const Date& d){
    const int y=d.year-(d.month<=2?1:0);
    const long era=(y>=0?y:y-399)/400;
    const unsigned yoe=unsigned(y-era*400);
    const unsigned doy=unsigned((153*(d.month+(d.month>2?-3:9))+2)/5+d.day-1);
    const unsigned doe=yoe*365+yoe/4-yoe/100+doy;
    return era*146097+long(doe)-719468;
}
inline Date fromDayNumber(long z){
    z+=719468;
    const long era=(z>=0?z:z-146096)/146097;
    const unsigned doe=unsigned(z-era*146097);
    const unsigned yoe=(doe-doe/1460+doe/36524-doe/146096)/365;
    const unsigned doy=doe-(365*yoe+yoe/4-yoe/100);
    const unsigned mp=(5*doy+2)/153;
    Date d; d.day=int(doy-(153*mp+2)/5+1); d.month=int(mp<10?mp+3:mp-9); d.year=int(long(yoe)+era*400+(d.month<=2?1:0));
    return d;
}
inline int weekday(const Date& d){long n=dayNumber(d)%7; if(n<0)n+=7; return int((n+4)%7);} // 0 = Sunday.
inline bool valid(const Date& d){return d.year>=MinYear && d.year<=MaxYear && d.month>=1 && d.month<=12 && d.day>=1 && d.day<=daysInMonth(d.year,d.month);}
inline Date clampDate(Date d){
    if(d.year<MinYear){d.year=MinYear;d.month=1;d.day=1;}
    if(d.year>MaxYear){d.year=MaxYear;d.month=12;d.day=31;}
    return d;
}
inline Date addDays(const Date& d,int days){return clampDate(fromDayNumber(dayNumber(d)+days));}
// Same day in another month, or that month's last day (Jan 31 + 1 month = Feb 28/29).
inline Date addMonths(const Date& d,int months){
    int index=d.year*12+(d.month-1)+months;
    Date r; r.year=index/12; r.month=index%12+1;
    const int last=daysInMonth(r.year,r.month); r.day=d.day<last?d.day:last;
    return clampDate(r);
}
// The month grid: Sunday-first weeks; "offset" blank cells before day 1; 4-6 rows.
struct MonthGrid { int year,month,offset,days,rows; };
inline MonthGrid monthGrid(int year,int month){
    Date first; first.year=year; first.month=month; first.day=1;
    MonthGrid g; g.year=year; g.month=month; g.offset=weekday(first); g.days=daysInMonth(year,month);
    g.rows=(g.offset+g.days+6)/7; return g;
}
inline const char* monthName(int m){
    static const char* const names[12]={"JANUARY","FEBRUARY","MARCH","APRIL","MAY","JUNE","JULY","AUGUST","SEPTEMBER","OCTOBER","NOVEMBER","DECEMBER"};
    return names[(m-1)%12];
}
inline const char* weekdayName(int w){static const char* const names[7]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};return names[w%7];}

// ---- Reminders (read-only view of Cache's reminder schedule) ----
// One reminder as Cache keeps it: its next due date (local), time and repeat.
struct ReminderRule { Date next; int hour=0,minute=0; ReminderRepeat repeat=ReminderRepeat::Once; bool active=false; };
// Whether a reminder falls on a day, with Cache's own repeat meanings: Once on its
// date; Daily every day from the next due date on; Weekly on the same weekday;
// Weekdays Monday to Friday. Past repeats are not kept by Cache, so they are not shown.
inline bool occursOn(const ReminderRule& r,const Date& day){
    if(!r.active) return false;
    const long d=dayNumber(day),start=dayNumber(r.next);
    if(d<start) return false;
    switch(r.repeat){
    case ReminderRepeat::Once: return d==start;
    case ReminderRepeat::Daily: return true;
    case ReminderRepeat::Weekly: return (d-start)%7==0;
    case ReminderRepeat::Weekdays: { const int w=weekday(day); return w!=0 && w!=6; }
    }
    return false;
}
inline const char* repeatName(ReminderRepeat r){
    switch(r){case ReminderRepeat::Daily:return "Daily";case ReminderRepeat::Weekly:return "Weekly";case ReminderRepeat::Weekdays:return "Weekdays";default:return "Once";}
}

// ---- Navigation ----
struct CalendarView {
    Date selected, today;
    bool todayKnown=false, detail=false;
    int detailTop=0;
    void open(const Date& now,bool known){
        todayKnown=known && valid(now); today=now; detail=false; detailTop=0;
        if(todayKnown) selected=now; else { selected=Date(); }
    }
    void move(int days){ selected=addDays(selected,days); detailTop=0; }
    void month(int delta){ selected=addMonths(selected,delta); detailTop=0; }
    bool goToday(){ if(!todayKnown) return false; selected=today; detailTop=0; return true; }
    // Esc: the day view closes first; false means leave Calendar.
    bool back(){ if(detail){ detail=false; return true; } return false; }
};
}
}
