#pragma once
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "../core/CalendarMath.h"
#include "../core/ClockFormat.h"
#include "../core/ReminderEngine.h"
#include "../ui/Screen.h"
namespace spark {
// Calendar (Fn+6): an offline month grid with Cache's reminders marked. Read-only:
// reminders are created, edited and deleted only in Cache. 1.3.0 restyles it in cream and
// gold (month grid with a selected-day panel, Enter for the full day list).
struct CalendarKeys { bool up=false,down=false,left=false,right=false,enter=false,prevMonth=false,nextMonth=false,today=false; };
namespace calendarColors {
// Cream and gold on black. The screen's strip buffer is RGB332, so these are picked
// to land exactly on its levels: cream is (255,255,170), gold (255,219,0).
constexpr uint16_t Cream=0xFFF5,Gold=0xFEC0,Muted=0x9495,Band=0x212A,Rule=0x4A4A,Background=theme::Background,Cyan=theme::Primary;
}
class CalendarApp {
public:
    calendar::CalendarView view;
    static constexpr int ColumnW=21,GridLeft=3,GridRight=150,PanelLeft=153,PanelTop=32,PanelBottom=121,DetailRows=7,DetailRowH=12;
    // Snapshot of Cache's reminders, taken before each frame (the screen draws in strips).
    struct Entry { calendar::ReminderRule rule; const char* title=""; };
    Entry entries[MaxNotes];
    unsigned count=0;
    void open(int64_t now,bool clockReady){
        view.open(localDate(now),clockReady && now>=ReminderClockMinimum);
    }
    static calendar::Date localDate(int64_t epoch){
        time_t t=time_t(epoch); struct tm local{}; localtime_r(&t,&local);
        calendar::Date d; d.year=local.tm_year+1900; d.month=local.tm_mon+1; d.day=local.tm_mday; return d;
    }
    // Reads the reminder schedule; called once per frame and on input, never stored back.
    void prepare(const ScheduledReminder* items,unsigned n,int64_t now,bool clockReady){
        count=0;
        if(clockReady && now>=ReminderClockMinimum){ view.today=localDate(now); view.todayKnown=calendar::valid(view.today); }
        for(unsigned i=0;i<n && count<MaxNotes;++i){
            const ScheduledReminder& item=items[i];
            if(!item.id[0] || !item.active || item.due<ReminderClockMinimum) continue;
            time_t t=time_t(item.due); struct tm local{}; localtime_r(&t,&local);
            Entry& e=entries[count++];
            e.rule.next.year=local.tm_year+1900; e.rule.next.month=local.tm_mon+1; e.rule.next.day=local.tm_mday;
            e.rule.hour=item.hour>=0?item.hour:local.tm_hour; e.rule.minute=item.minute>=0?item.minute:local.tm_min;
            e.rule.repeat=item.repeat; e.rule.active=true; e.title=item.text;
        }
    }
    unsigned remindersOn(const calendar::Date& day) const {
        unsigned n=0; for(unsigned i=0;i<count;++i) if(calendar::occursOn(entries[i].rule,day)) ++n; return n;
    }
    // The selected day's reminders in time order (indexes into entries).
    unsigned dayList(unsigned* out,unsigned max) const {
        unsigned n=0;
        for(unsigned i=0;i<count && n<max;++i) if(calendar::occursOn(entries[i].rule,view.selected)) out[n++]=i;
        for(unsigned a=1;a<n;++a) for(unsigned b=a;b>0 && minuteOf(out[b])<minuteOf(out[b-1]);--b){ unsigned t=out[b];out[b]=out[b-1];out[b-1]=t; }
        return n;
    }
    // True when the screen needs a redraw.
    bool input(const CalendarKeys& k){
        if(view.detail){
            unsigned list[MaxNotes]; const int n=int(dayList(list,MaxNotes));
            if(k.up && view.detailTop>0){ --view.detailTop; return true; }
            if(k.down && view.detailTop+DetailRows<n){ ++view.detailTop; return true; }
            return false;
        }
        if(k.left){ view.move(-1); return true; }
        if(k.right){ view.move(1); return true; }
        if(k.up){ view.move(-7); return true; }
        if(k.down){ view.move(7); return true; }
        if(k.prevMonth){ view.month(-1); return true; }
        if(k.nextMonth){ view.month(1); return true; }
        if(k.today) return view.goToday();
        if(k.enter){ view.detail=true; view.detailTop=0; return true; }
        return false;
    }
    bool back(){ return view.back(); }
    void draw() const { if(view.detail) drawDay(); else drawMonth(); }
private:
    int minuteOf(unsigned i) const { return entries[i].rule.hour*60+entries[i].rule.minute; }
    static void textOn(int x,int y,const char* value,uint16_t color,uint16_t background){
        auto& d=screen(); d.setTextSize(1); d.setTextColor(color,background); d.setCursor(x,y); d.print(value);
    }
    static int widthOf(const char* value){auto& d=screen();d.setTextSize(1);return d.textWidth(value)-1;}
    // "idkOS [V1] | CALENDAR" with a gold rule; the status bar draws Wi-Fi and battery from x=142.
    static void header(){
        using namespace calendarColors; auto& d=screen(); d.fillScreen(Background);
        text(4,3,"idk",1,Cream); text(22,3,"OS",1,Cyan);
        d.drawRoundRect(38,1,17,11,2,Gold); text(41,3,"V1",1,Gold);
        d.drawFastVLine(60,2,9,Rule); text(66,3,"CALENDAR",1,Cream);
        d.fillRect(0,13,240,2,Gold);
    }
    // Footer key hints, centred: keys in gold, words in cream.
    static void footer(const char* const* parts,int n){
        using namespace calendarColors; auto& d=screen();
        d.fillRect(0,122,240,1,Gold);
        int w=0; for(int i=0;i<n;++i) w+=widthOf(parts[i])+1+(i&1?10:4);
        w-=10; int x=120-w/2;
        for(int i=0;i<n;++i){ textOn(x,126,parts[i],i&1?Cream:Gold,Background); x+=widthOf(parts[i])+1+(i&1?10:4); }
    }
    static void calendarIcon(int x,int y){
        using namespace calendarColors; auto& d=screen();
        d.fillRoundRect(x,y+2,15,13,2,Gold); d.fillRect(x+2,y+6,11,7,Background);
        d.fillRect(x+3,y,2,4,Gold); d.fillRect(x+10,y,2,4,Gold);
        for(int r=0;r<2;++r) for(int c=0;c<3;++c) d.fillRect(x+3+c*4,y+7+r*3,2,2,Gold);
    }
    static void arrowBox(int x,int y,bool left){
        using namespace calendarColors; auto& d=screen();
        d.drawRoundRect(x,y,13,13,3,Cream);
        if(left) d.fillTriangle(x+4,y+6,x+8,y+3,x+8,y+9,Cream); else d.fillTriangle(x+8,y+6,x+4,y+3,x+4,y+9,Cream);
    }
    static void shortDate(char* out,size_t n,const calendar::Date& day){
        snprintf(out,n,"%.3s %d, %d",calendar::monthName(day.month),day.day,day.year);
    }
    void drawMonth() const {
        using namespace calendarColors; auto& d=screen();
        header();
        const calendar::MonthGrid g=calendar::monthGrid(view.selected.year,view.selected.month);
        // Month bar: [<]  NOV 2026  [>]   ([ and ] change month).
        char line[40]; snprintf(line,sizeof line,"%.3s %d",calendar::monthName(g.month),g.year);
        d.drawRoundRect(84,17,72,13,4,Cream); centerText(120,20,line,1,Cream);
        arrowBox(66,17,true); arrowBox(161,17,false);
        // Month grid.
        d.drawRoundRect(0,PanelTop,GridRight+1,PanelBottom-PanelTop+1,4,Cream);
        static const char* const days[7]={"SU","MO","TU","WE","TH","FR","SA"};
        for(int c=0;c<7;++c) centerText(GridLeft+c*ColumnW+ColumnW/2,35,days[c],1,Cream);
        d.drawFastHLine(5,44,GridRight-9,Gold);
        const int top=47,rowH=g.rows>5?12:g.rows>4?14:17;
        for(int cell=0;cell<g.rows*7;++cell){
            const int day=cell-g.offset+1; if(day<1 || day>g.days) continue;
            const int col=cell%7,row=cell/7,x=GridLeft+col*ColumnW,y=top+row*rowH;
            calendar::Date date; date.year=g.year; date.month=g.month; date.day=day;
            const bool on=date==view.selected,isToday=view.todayKnown && date==view.today;
            if(on) d.fillRoundRect(x+1,y,ColumnW-2,rowH-1,2,Gold);
            else if(isToday) d.drawRoundRect(x+1,y,ColumnW-2,rowH-1,2,Gold);
            char num[12]; snprintf(num,sizeof num,"%d",day);
            const int cx=x+ColumnW/2,ty=y+(rowH-1-8)/2;
            centerText(cx,ty,num,1,on?Background:isToday?Gold:Cream,on?Gold:Background);
            // A reminder that day: a small dot under the number.
            if(remindersOn(date)) d.fillRect(cx-1,ty+9,2,rowH>12?2:1,on?Background:Cyan);
        }
        // Selected day panel.
        d.drawRoundRect(PanelLeft,PanelTop,240-PanelLeft,PanelBottom-PanelTop+1,4,Cream);
        const int pc=PanelLeft+(240-PanelLeft)/2;
        calendarIcon(PanelLeft+5,36);
        d.drawFastVLine(PanelLeft+25,36,15,Rule);
        char wd[4]; snprintf(wd,sizeof wd,"%.3s",calendar::weekdayName(calendar::weekday(view.selected)));
        for(char* c=wd;*c;++c) if(*c>='a'&&*c<='z') *c=char(*c-32);
        centerText(PanelLeft+25+(240-PanelLeft-25)/2,36,wd,2,Cream);
        shortDate(line,sizeof line,view.selected); centerText(pc,55,line,1,Cream);
        d.fillRect(PanelLeft+4,65,240-PanelLeft-8,1,Gold);
        d.fillRoundRect(PanelLeft+3,68,240-PanelLeft-6,11,2,Band); centerText(pc,70,"REMINDERS",1,Cream,Band);
        if(!view.todayKnown){ centerText(pc,86,"Clock not",1,theme::Attention); centerText(pc,96,"set",1,theme::Attention); }
        else {
            unsigned list[MaxNotes]; const unsigned n=dayList(list,MaxNotes);
            if(!n) centerText(pc,90,"None",1,Muted);
            for(unsigned r=0;r<n && r<2;++r){
                const int y=83+int(r)*19;
                if(r==1 && n>2){ snprintf(line,sizeof line,"+%u more",n-1); centerText(pc,y+4,line,1,Gold); break; }
                const Entry& e=entries[list[r]];
                d.drawRect(PanelLeft+5,y+1,6,6,Gold);
                char title[12]{}; strncpy(title,e.title[0]?e.title:"(untitled)",sizeof title-1);
                textOn(PanelLeft+14,y,title,Cream,Background);
                char clock[9]; formatClock(clock,sizeof clock,e.rule.hour,e.rule.minute); textOn(PanelLeft+14,y+9,clock,Gold,Background);
                if(r==0 && n>1) d.fillRect(PanelLeft+4,y+18,240-PanelLeft-8,1,Rule);
            }
        }
        static const char* const keys[]={"[ ]","MONTH","T","TODAY","ENTER","DAY","ESC","BACK"};
        footer(keys,8);
    }
    void drawDay() const {
        using namespace calendarColors; auto& d=screen();
        header();
        char line[48]; shortDate(line,sizeof line,view.selected);
        char bar[64]; snprintf(bar,sizeof bar,"%s, %s",calendar::weekdayName(calendar::weekday(view.selected)),line);
        for(char* c=bar;*c && *c!=',';++c) if(*c>='a'&&*c<='z') *c=char(*c-32);
        d.drawRoundRect(20,17,200,13,4,Cream); centerText(120,20,bar,1,Cream);
        d.drawRoundRect(0,PanelTop,240,PanelBottom-PanelTop+1,4,Cream);
        unsigned list[MaxNotes]; const unsigned n=dayList(list,MaxNotes);
        if(!n){ calendarIcon(112,52); centerText(120,74,"No reminders on this day.",1,Muted); centerText(120,88,"Add them in Cache",1,Muted); }
        for(int r=0;r<DetailRows && view.detailTop+r<int(n);++r){
            const Entry& e=entries[list[view.detailTop+r]]; const int y=36+r*DetailRowH;
            d.drawRect(5,y,7,7,Gold);
            char clock[9]; formatClock(clock,sizeof clock,e.rule.hour,e.rule.minute); textOn(16,y,clock,Gold,Background);
            char title[19]{}; strncpy(title,e.title[0]?e.title:"(untitled)",sizeof title-1); textOn(66,y,title,Cream,Background);
            const char* rep=calendar::repeatName(e.rule.repeat); textOn(234-widthOf(rep),y,rep,Muted,Background);
            if(r+1<DetailRows && view.detailTop+r+1<int(n)) d.fillRect(4,y+10,232,1,Rule);
        }
        if(int(n)>DetailRows){
            static const char* const keys[]={"UP/DOWN","SCROLL","ESC","BACK"}; footer(keys,4);
            const int last=view.detailTop+DetailRows<int(n)?view.detailTop+DetailRows:int(n);
            snprintf(line,sizeof line,"%d-%d/%u",view.detailTop+1,last,n); textOn(236-widthOf(line),126,line,Muted,Background);
        } else { static const char* const keys[]={"ESC","BACK"}; footer(keys,2); }
    }
};
}
