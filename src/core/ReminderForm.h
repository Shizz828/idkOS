#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "Input.h"
#include "ClockFormat.h"
#include "NoteReminder.h"
#include "ReminderEngine.h"
namespace spark {
// Reminder editor with separate Time, Date, Repeat and Active fields, so editing
// one keeps the others. Values stay local wall-clock parts until saved; the
// stored YYYY-MM-DD HH:MM text and its validation are unchanged.
class ReminderForm {
    int saved[5]{};
    char typed[5]{};
    static int daysIn(int month,int year){
        const int days[]={31,28,31,30,31,30,31,31,30,31,30,31};
        bool leap=(year%4==0 && year%100!=0) || year%400==0;
        return month==2 && leap?29:days[(month-1)%12];
    }
    unsigned width(unsigned p)const{return field==Date && p==2?4:2;}
    void clampDay(){int last=daysIn(month,year);if(day>last)day=last;}
    // Typed digits replace the selected part once they form a valid value.
    void apply(int value){
        if(field==Time && part==0){
            if(twelveHourClock()){if(value>=1 && value<=12)hour=hour/12*12+value%12;}
            else if(value>=0 && value<=23)hour=value;
        }
        else if(field==Time && part==1){if(value>=0 && value<=59)minute=value;}
        else if(field==Date && part==0){if(value>=1 && value<=12){month=value;clampDay();}}
        else if(field==Date && part==1){if(value>=1 && value<=daysIn(month,year))day=value;}
        else if(field==Date && part==2){if(value>=2024 && value<=2100){year=value;clampDay();}}
    }
    void commit(){if(typed[0])apply(atoi(typed));typed[0]=0;}
    void adjust(int delta){
        if(field==Time && part==0){
            if(twelveHourClock())hour=hour/12*12+(hour%12+delta+12)%12;
            else hour=(hour+delta+24)%24;
        }
        else if(field==Time && part==1)minute=(minute+delta+60)%60;
        else if(field==Time && part==2)hour=(hour+12)%24; // AM/PM
        else if(field==Date && part==0){month=(month-1+delta+12)%12+1;clampDay();}
        else if(field==Date && part==1){int last=daysIn(month,year);day=(day-1+delta+last)%last+1;}
        else if(field==Date && part==2){year+=delta;if(year<2024)year=2024;if(year>2100)year=2100;clampDay();}
    }
public:
    enum Field { Time,Date,Repeat,Active,Save,FieldCount };
    enum class Action { None,Save };
    int hour=9,minute=0,month=1,day=1,year=2026;
    ReminderRepeat repeat=ReminderRepeat::Once;bool active=true;
    unsigned field=Time,part=0;bool editing=false;
    void load(int64_t due,ReminderRepeat r,bool on){
        time_t epoch=due;struct tm local{};localtime_r(&epoch,&local);
        hour=local.tm_hour;minute=local.tm_min;month=local.tm_mon+1;day=local.tm_mday;year=local.tm_year+1900;
        if(year<2024 || year>2100){year=2026;month=1;day=1;hour=9;minute=0;}
        repeat=r;active=on;field=Time;part=0;editing=false;typed[0]=0;
    }
    unsigned parts()const{return field==Time?(twelveHourClock()?3:2):field==Date?3:0;}
    void stored(char* out,size_t size)const{snprintf(out,size,"%04d-%02d-%02d %02d:%02d",year,month,day,hour,minute);}
    // 0 when the parts are not a real local time (e.g. a DST gap).
    int64_t due()const{char text[24];stored(text,sizeof(text));return parseReminderDate(text);}
    Action input(const InputEvent& e){
        if(!editing){
            if(e.up)field=(field+FieldCount-1)%FieldCount;
            if(e.down)field=(field+1)%FieldCount;
            if(field==Repeat && (e.left || e.right))repeat=ReminderRepeat((unsigned(repeat)+(e.left?3:1))%4);
            if(field==Active && (e.left || e.right))active=!active;
            if(e.enter){
                if(field==Time || field==Date){
                    editing=true;part=0;typed[0]=0;
                    const int values[]={hour,minute,month,day,year};memcpy(saved,values,sizeof(saved));
                }
                else if(field==Repeat)repeat=ReminderRepeat((unsigned(repeat)+1)%4);
                else if(field==Active)active=!active;
                else return Action::Save;
            }
            return e.save?Action::Save:Action::None;
        }
        if(e.erase && typed[0])typed[strlen(typed)-1]=0;
        if(e.left || e.right){commit();part=(part+(e.left?parts()-1:1))%parts();}
        if(e.up || e.down){commit();adjust(e.up?1:-1);}
        for(const char* c=e.text;*c;++c){
            if(field==Time && part==2){
                if(*c=='a' || *c=='A')hour%=12;
                if(*c=='p' || *c=='P')hour=hour%12+12;
            }
            else if(*c>='0' && *c<='9'){
                size_t n=strlen(typed);typed[n]=*c;typed[n+1]=0;
                if(n+1==width(part)){commit();if(part+1<parts())++part;}
            }
        }
        if(e.enter || e.save){commit();editing=false;}
        return e.save?Action::Save:Action::None;
    }
    // Esc while editing restores only this field; otherwise the caller closes the form.
    bool back(){
        if(!editing)return false;
        hour=saved[0];minute=saved[1];month=saved[2];day=saved[3];year=saved[4];
        editing=false;typed[0]=0;return true;
    }
    // Field text and the character span of each part, for the edit underline.
    void text(unsigned f,char* out,size_t size,int start[3],int length[3])const{
        char p[3][6];int w[3]={2,2,0};
        if(f==Time){
            int shown=hour;if(twelveHourClock()){shown=hour%12;if(!shown)shown=12;}
            snprintf(p[0],sizeof(p[0]),twelveHourClock()?"%d":"%02d",shown);snprintf(p[1],sizeof(p[1]),"%02d",minute);
            snprintf(p[2],sizeof(p[2]),"%s",hour<12?"AM":"PM");w[2]=twelveHourClock()?2:0;
        } else {
            snprintf(p[0],sizeof(p[0]),"%02d",month);snprintf(p[1],sizeof(p[1]),"%02d",day);snprintf(p[2],sizeof(p[2]),"%04d",year);w[2]=4;
        }
        // While digits are being typed, show them in place of the part.
        if(editing && f==field && typed[0] && !(f==Time && part==2)){
            unsigned n=strlen(typed);for(unsigned i=0;i<width(part);++i)p[part][i]=i<n?typed[i]:'_';p[part][width(part)]=0;
        }
        if(f==Time)snprintf(out,size,w[2]?"%s:%s %s":"%s:%s",p[0],p[1],p[2]);
        else snprintf(out,size,"%s/%s/%s",p[0],p[1],p[2]);
        int at=0;
        for(int i=0;i<3;++i){start[i]=at;length[i]=w[2]||i<2?int(strlen(p[i])):0;at+=length[i]+1;}
    }
};
}
