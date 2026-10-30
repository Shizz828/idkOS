#include <cassert>
#include <cstring>
#include "../src/apps/WifiSetup.h"
struct Network {
 unsigned scans=0; bool available=true;
 bool networkAction(uint8_t action,unsigned=0){assert(action==4);++scans;return available;}
 unsigned calls=0; char password[65]{};
 uint32_t connect(const char* ssid,const char* pass) {
  assert(!strcmp(ssid,"Test network")); strcpy(password,pass); return ++calls;
 }
};
int main() {
 spark::WifiSetup form; Network service; spark::InputEvent e;
 form.open(); strcpy(e.text,"Test network"); form.input(e,service);
 e={}; e.enter=true; form.input(e,service);
 e={}; strcpy(e.text,"testpass"); form.input(e,service);
 e={}; e.enter=true; form.input(e,service);
 assert(form.editing && service.calls==1); // Queueing must not close the form.
 form.input(e,service); assert(service.calls==1); // No double submission.
 spark::WeatherSnapshot state;
 state.wifi.start(99,0); form.sync(state); assert(form.editing); // Ignore other attempts.
 state.wifi.start(1,0); form.sync(state); assert(form.editing);
 state.wifi.update(false,202,1); form.sync(state); assert(form.editing);
 form.draw(state); // Error and masked field remain within bounds.
 form.input(e,service); assert(service.calls==2); // Retry after failure.
 state.wifi.start(2,0); state.wifi.update(true,0,1); form.sync(state);
 assert(!form.editing); // Only successful connection closes it.
 form.open(); form.input(e,service); form.input(e,service);
 assert(service.calls==3 && !service.password[0]); // Success wiped the password.
 form.cancel(); assert(!form.editing);
 // Settings owns the global network picker and credential form.
 state={};form.open(service,state);assert(service.scans==1);form.draw(state);
 e={};e.enter=true;form.input(e,service);assert(service.calls==3);
 state.scanning=true;form.sync(state);form.draw(state);
 state.scanning=false;state.scanRevision=1;state.nearbyCount=12;
 for(unsigned i=0;i<12;++i){strcpy(state.nearby[i].ssid,"Test network");state.nearby[i].rssi=-100;}
 form.sync(state);form.draw(state);
 for(int i=0;i<11;++i){e={};e.down=true;form.input(e,service);}form.draw(state);
 e={};e.enter=true;form.input(e,service); // Selected SSID skips to password.
 e={};strcpy(e.text,"testpass");form.input(e,service);
 e={};e.enter=true;form.input(e,service);assert(service.calls==4);
 state.wifi.start(4,0);state.wifi.update(false,202,1);form.sync(state);form.draw(state);
 form.input(e,service);assert(service.calls==5);
 state.wifi.start(5,0);state.wifi.update(true,0,1);form.sync(state);assert(!form.editing);
 form.open(service,state);state.scanRevision=2;state.nearbyCount=0;state.scanFailed=true;
 form.sync(state);form.draw(state);
 e={};e.text[0]='r';form.input(e,service);assert(service.scans==3);
 state.scanRevision=3;state.scanFailed=false;form.sync(state);form.draw(state);
 service.available=false;form.input(e,service);form.draw(state);
 e={};e.text[0]='m';form.input(e,service);form.draw(state);
 e={};strcpy(e.text,"Test network");form.input(e,service);
 e={};e.enter=true;form.input(e,service);form.input(e,service);
 assert(service.calls==6 && !service.password[0]); // Manual open network.
 form.cancel();form.open(service,state);form.cancel();
 state.scanRevision=4;form.sync(state);assert(!form.editing);
}
