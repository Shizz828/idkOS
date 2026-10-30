#include <cassert>
#include "../src/apps/LocationSettings.h"
struct Service {
    unsigned calls=0; bool saveHome=false; spark::LocationMode mode=spark::LocationMode::Auto; spark::WeatherLocation value;
    bool configureLocation(spark::LocationMode m,const spark::WeatherLocation& l,bool home=false) { ++calls; mode=m; value=l; saveHome=home; return true; }
};
void type(spark::LocationSettings& form,Service& service,const char* text,bool enter=false) {
    spark::InputEvent event; strcpy(event.text,text); event.enter=enter; form.input(event,service);
}
int main() {
    Service service; spark::LocationSettings form; spark::WeatherSnapshot state;
    form.open(); type(form,service,"a"); assert(!form.active && service.mode==spark::LocationMode::Auto);
    form.open(); type(form,service,"h"); assert(!form.active && service.mode==spark::LocationMode::Home);
    form.open(); type(form,service,"m"); type(form,service,"New York, NY",true);
    type(form,service,"40.7128",true); type(form,service,"-74.0060");
    spark::InputEvent save; save.save=true; form.input(save,service);
    assert(!form.active && service.saveHome && service.mode==spark::LocationMode::Manual);
    assert(service.value.latitude==40.7128 && !strcmp(service.value.label,"New York, NY"));
    unsigned calls=service.calls; form.open(); type(form,service,"m"); type(form,service,"Invalid",true);
    type(form,service,"nan",true); type(form,service,"0",true);
    assert(form.active && service.calls==calls); form.draw(state);
    form.cancel(); form.open(); type(form,service,"m"); type(form,service,"Invalid",true);
    type(form,service,"91",true); type(form,service,"0",true);
    assert(form.active && service.calls==calls); form.draw(state);
}
