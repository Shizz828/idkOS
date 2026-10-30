#include <cassert>
#include <string>
#include "../src/apps/SshApp.h"
using namespace spark;
struct Service {
 SshSnapshot state;bool failStart=false,busyStart=false;const char* failure="SSH RAM low: 48K free, 20K block";bool running=false;unsigned connects=0,disconnects=0;int choice=0,scrollAmount=0;std::string password,sent;
 bool snapshot(SshSnapshot& out){out=state;return true;}bool busy(){return running;}
 const char* startError()const{return failure;}
 bool connect(const SshProfile& profile,const char* secret){
 if(busyStart)return false;
 if(failStart){state.phase=SshPhase::Failed;strcpy(state.message,failure);++state.revision;return false;}
 ++connects;running=true;state.phase=SshPhase::Connecting;strcpy(state.message,"Connecting to host");state.profile=profile;++state.revision;password=secret;return true;}
 void disconnect(){++disconnects;}void verify(int c){choice=c;}void scroll(int n){scrollAmount+=n;}
 bool send(const char* bytes,unsigned size){sent.append(bytes,size);return true;}
 void phase(SshPhase phase){state.phase=phase;++state.revision;}
};
bool shown(const char* text){for(const auto& line:M5Cardputer.Display.printed)if(line.text.find(text)!=std::string::npos)return true;return false;}
void type(SshApp& app,Service& service,const char* value,bool online=true){InputEvent e;sshCopy(e.text,value,sizeof(e.text));app.input(e,service,online);}
void enter(SshApp& app,Service& service,bool online=true){InputEvent e;e.enter=true;app.input(e,service,online);}
int main(){
 {
 Preferences::data().clear();
 Input input;uint32_t ticks=0;
 auto physicalEnter=[&](SshApp& app,Service& service){
  auto& keyboard=M5Cardputer.Keyboard;keyboard.s={};keyboard.keys={{13,2}};
  InputEvent e=input.poll(ticks+=50,true);assert(e.enter);
  app.input(e,service,true);keyboard.keys.clear();input.poll(++ticks,true);
 };
 SshApp numeric;Service host;numeric.open(host);
 type(numeric,host,"192.0.2.10");physicalEnter(numeric,host);physicalEnter(numeric,host);
 type(numeric,host,"qcuser");physicalEnter(numeric,host);type(numeric,host,"test-secret");physicalEnter(numeric,host);
 assert(host.connects==1 && !strcmp(host.state.profile.host,"192.0.2.10") && !strcmp(host.state.profile.user,"qcuser") && host.password=="test-secret");
 // A genuinely missing user identifies that field and preserves the other inputs.
 SshApp missing;Service retry;missing.open(retry);
 type(missing,retry,"192.0.2.10");physicalEnter(missing,retry);physicalEnter(missing,retry);physicalEnter(missing,retry);
 type(missing,retry,"test-secret");physicalEnter(missing,retry);missing.draw();assert(!retry.connects && shown("Username is required"));
 type(missing,retry,"qcuser");physicalEnter(missing,retry);physicalEnter(missing,retry);
 assert(retry.connects==1 && retry.password=="test-secret");
 Preferences::data().clear();
 }
 Preferences::data().clear();SshApp app;Service service;app.open(service);app.draw();assert(shown("Host/IP:"));
 type(app,service,"linux.example");enter(app,service);enter(app,service);type(app,service,"student");enter(app,service);type(app,service,"test-secret");app.draw();assert(!shown("test-secret") && shown("***********"));
 enter(app,service,false);app.draw();assert(shown("Wi-Fi unavailable") && !service.connects);
 enter(app,service);assert(service.connects==1 && service.password=="test-secret");app.poll(service);app.draw();assert(shown("Connecting"));
 service.phase(SshPhase::Verify);strcpy(service.state.fingerprint,"SHA256:AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");app.poll(service);app.draw();assert(shown("VERIFY SERVER KEY"));assert(shown("AAAAAAAAAAAA"));
 type(app,service,"y");assert(service.choice==1);type(app,service,"o");assert(service.choice==2);enter(app,service);assert(service.choice==-1);
 service.phase(SshPhase::Connected);app.poll(service);app.draw();assert(shown("SSH: Connected"));
 type(app,service,"pwd");enter(app,service);assert(service.sent=="pwd\r");InputEvent control;control.control=3;app.input(control,service,true);assert(service.sent.back()==3);
 InputEvent arrow;arrow.up=true;app.input(arrow,service,true);assert(service.sent.substr(service.sent.size()-3)=="\033[A");arrow.shift=true;unsigned before=service.sent.size();app.input(arrow,service,true);assert(service.sent.size()==before);
 assert(app.back(service));app.draw();assert(shown("Disconnect SSH session? [y/N]"));enter(app,service);assert(service.disconnects==0);
 assert(app.back(service));type(app,service,"n");assert(service.disconnects==0);
 assert(app.back(service));assert(app.back(service));assert(service.disconnects==0); // Esc declines confirmation.
 assert(app.back(service));type(app,service,"y");assert(service.disconnects==1);
 app.leave();assert(service.running);app.open(service);app.draw();assert(shown("SSH: Connected")); // Home/reopen keeps it alive.
 service.running=false;service.phase(SshPhase::Disconnected);strcpy(service.state.message,"Remote shell closed");app.poll(service);app.draw();assert(shown("Remote shell closed") && !shown("***********"));
 // Saving from the form stores only named connection details, never the secret.
 InputEvent up;up.up=true;for(unsigned i=0;i<4;++i)app.input(up,service,true);type(app,service,"Linux");InputEvent save;save.save=true;app.input(save,service,true);app.draw();assert(shown("Profile saved"));
 for(const auto& item:Preferences::data()){std::string raw(item.second.begin(),item.second.end());assert(raw.find("test-secret")==std::string::npos);}
 assert(app.back(service));app.draw();assert(shown("SAVED SSH HOSTS"));enter(app,service);type(app,service,"new-secret");app.leave();app.open(service);enter(app,service);app.draw();assert(!shown("**********"));
 assert(app.back(service));InputEvent del;del.control=4;app.input(del,service,true);app.draw();assert(shown("Remove saved SSH profile"));type(app,service,"y");assert(!app.back(service));
 // An allocation failure before the first worker tick must stay visible,
 // including after polling the failed snapshot. Retry must request a new secret.
 SshApp failed;Service low;low.failStart=true;failed.open(low);
 type(failed,low,"server.example");enter(failed,low);enter(failed,low);type(failed,low,"user");enter(failed,low);type(failed,low,"never-store-this");enter(failed,low);
 failed.draw();assert(shown("SSH RAM low: 48K free, 20K block"));
 failed.poll(low);failed.draw();assert(shown("SSH RAM low: 48K free, 20K block"));assert(!shown("********"));
 low.failStart=false;enter(failed,low);assert(low.password.empty());
 // A busy result with a live snapshot opens that session so Esc can cancel it.
 SshApp busy;Service active;active.failStart=true;busy.open(active);
 type(busy,active,"server.example");enter(busy,active);enter(busy,active);type(busy,active,"user");enter(busy,active);
 active.failStart=false;active.running=true;active.state.phase=SshPhase::Connected;active.failure="SSH worker busy; retry shortly";
 // Model an already-running connection becoming visible during submission.
 active.failStart=false;active.busyStart=true;enter(busy,active);busy.draw();assert(shown("SSH: Connected"));assert(busy.back(active));busy.draw();assert(shown("Disconnect SSH session?"));
 // All five profiles and long fields/fingerprints remain inside the ADV display.
 SshProfiles profiles;profiles.begin();SshProfile p;memset(p.name,'N',24);p.name[24]=0;memset(p.host,'a',96);p.host[96]=0;memset(p.user,'u',32);p.user[32]=0;
 for(unsigned i=0;i<5;++i){p.name[0]=char('A'+i);assert(profiles.put(p));}
 SshApp five;Service remote;five.open(remote);InputEvent down;down.down=true;for(unsigned i=0;i<4;++i)five.input(down,remote,true);five.draw();assert(shown("ENNN"));enter(five,remote);five.draw();enter(five,remote);five.poll(remote);remote.phase(SshPhase::Verify);strcpy(remote.state.fingerprint,"SHA256:AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");five.poll(remote);five.draw();assert(shown("VERIFY SERVER KEY"));
}
