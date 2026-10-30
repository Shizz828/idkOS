#include <initializer_list>
#include <cstdlib>
#include "../src/ui/StatusBar.h"
#include "../src/ui/ReminderNotice.h"
#include "../src/apps/LocationSettings.h"
int main() {
 for(bool twelve:{false,true}){spark::twelveHourClock()=twelve;

 spark::StatusState s; s.online=true;
 for(int battery:{-1,0,15,16,50,100}) { s.battery=battery; spark::drawStatus(s); s.online=false; spark::drawStatus(s); }
 setenv("TZ","EST5EDT,M3.2.0/2,M11.1.0/2",1);tzset();
 auto now=spark::parseReminderDate("2026-10-12 23:59");assert(spark::statusClockMinute(now,true)==1439);assert(spark::statusClockMinute(now,false)==-1);assert(spark::statusClockMinute(0,true)==-1);
 for(int minute:{-1,0,1,59,60,720,1439}){s.clockMinute=minute;spark::drawStatus(s);}
 spark::frame("LOCAL CONSOLE");spark::drawStatus(s);
 spark::ReminderEngine engine; strcpy(engine.record.notice.text,"Water the plants");
 engine.record.notice.active=true; engine.record.notice.due=1704067200;
 spark::drawReminderNotice(engine,false); spark::drawReminderNotice(engine,true);
 engine.record.notice.work=true;spark::drawReminderNotice(engine,false);
 spark::WeatherSnapshot weather; spark::LocationSettings settings; settings.open(); settings.draw(weather);

 }
}
