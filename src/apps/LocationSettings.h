#pragma once
#include "../core/Input.h"
#include "../services/WeatherData.h"
#include "../ui/Screen.h"
#include <stdlib.h>
namespace spark {
class LocationSettings {
    unsigned field=0;
    char city[28]{},latitude[18]{},longitude[18]{};
    const char* message="A Auto  H Home  M Manual";
public:
    bool active=false;
    void open() { active=true; field=0; message="A Auto  H Home  M Manual"; }
    void cancel() { active=false; }
    template<typename Service>
    void input(const InputEvent& e,Service& service) {
        if(!field) {
            for(const char* c=e.text;*c;++c) {
                if(*c=='m' || *c=='M') { field=1; city[0]=latitude[0]=longitude[0]=0; message="City/state; Enter advances"; }
                else if(*c=='a' || *c=='A' || *c=='h' || *c=='H') {
                    WeatherLocation home;
                    if(service.configureLocation((*c=='a'||*c=='A')?LocationMode::Auto:LocationMode::Home,home)) active=false;
                    else message="Location setup busy; retry";
                }
            }
            return;
        }
        char* target=field==1?city:field==2?latitude:longitude;
        unsigned capacity=field==1?sizeof(city):sizeof(latitude); size_t n=strlen(target);
        if(e.erase && n) target[--n]=0;
        for(const char* c=e.text;*c && n+1<capacity;++c) { target[n++]=*c; target[n]=0; }
        if(e.up && field>1) --field;
        if(e.down && field<3) ++field;
        if(e.enter || e.save) {
            if(field<3) { if(n) ++field; return; }
            char *latEnd,*lonEnd; WeatherLocation value;
            value.latitude=strtod(latitude,&latEnd); value.longitude=strtod(longitude,&lonEnd);
            strlcpy(value.label,city,sizeof(value.label));
            if(!latitude[0] || !longitude[0] || *latEnd || *lonEnd || !validLocation(value)) message="Invalid coordinates / city";
            else if(service.configureLocation(LocationMode::Manual,value,e.save)) active=false;
            else message="Location setup busy; retry";
        }
    }
    void draw(const WeatherSnapshot& state) const {
        frame("LOCATION");
        if(!field) {
            const char* mode=state.locationMode==LocationMode::Auto?"AUTO (IP approximate)":state.locationMode==LocationMode::Home?"HOME":"MANUAL";
            text(6,34,mode,1,theme::Primary); text(6,50,state.location.label);
            text(6,72,"A Auto  H Home  M Manual");
            text(6,94,"Home/fallback saved locally"); text(6,126,"Esc back"); return;
        }
        text(6,32,field==1?"> City/state":"City/state",1,theme::Primary); text(6,43,city);
        text(6,58,field==2?"> Latitude":"Latitude",1,theme::Primary); text(6,69,latitude);
        text(6,84,field==3?"> Longitude":"Longitude",1,theme::Primary); text(6,95,longitude);
        text(6,112,message); text(6,126,"Enter manual  Ctrl+S save Home");
    }
};
}
