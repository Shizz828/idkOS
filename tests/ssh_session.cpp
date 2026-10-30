#include <cassert>
#include <string>
#include "../src/ssh/SshSession.h"
using namespace spark;
struct Transport {
 SshIo connectResult=SshIo::Ok,authResult=SshIo::Ok,handshakeResult=SshIo::Ok,openResult=SshIo::Ok;
 int authCalls=0,opens=0;bool ended=false,validFingerprint=true;std::string sent,received;int maxWrite=2;
 SshIo connect(const SshProfile&){return connectResult;}SshIo handshake(){return handshakeResult;}
 bool fingerprint(char* output,size_t size){sshCopy(output,("SHA256:"+std::string(43,'A')).c_str(),size);return validFingerprint;}
 SshIo authenticate(const char* password){++authCalls;assert(!strcmp(password,"secret"));return authResult;}
 SshIo openShell(){++opens;return openResult;}
 int write(const char* data,unsigned n){unsigned count=n<unsigned(maxWrite)?n:unsigned(maxWrite);sent.append(data,count);return int(count);}
 int read(char* output,unsigned size,unsigned stream){if(stream || received.empty())return 0;unsigned n=received.size()<size?unsigned(received.size()):size;memcpy(output,received.data(),n);received.erase(0,n);return int(n);}
 bool eof(){return ended;}const char* error(){return "Host unreachable";}
};
struct Trust {SshTrust known=SshTrust::Unknown;bool saveOk=true;int saves=0;SshTrust check(const SshProfile&,const char*){return known;}bool remember(const SshProfile&,const char*){++saves;return saveOk;}};
SshProfile profile(){SshProfile p;strcpy(p.host,"Linux.EXAMPLE");strcpy(p.user,"user");return p;}
int main(){
 SshSession session;Transport t;Trust trust;SshInputBuffer input;session.begin(profile(),"secret",0);
 session.tick(1,true,false,0,t,trust,input);session.tick(2,true,false,0,t,trust,input);assert(session.state.phase==SshPhase::Verify && t.authCalls==0);
 session.tick(3,true,false,0,t,trust,input);assert(t.authCalls==0);
 trust.saveOk=false;session.tick(4,true,false,1,t,trust,input);assert(session.state.phase==SshPhase::Verify && t.authCalls==0);
 session.tick(5,true,false,2,t,trust,input);session.tick(6,true,false,0,t,trust,input);session.tick(7,true,false,0,t,trust,input);assert(session.state.phase==SshPhase::Connected && t.authCalls==1);
 assert(input.push("pwd\r",4));for(unsigned now=8;now<11;++now)session.tick(now,true,false,0,t,trust,input);assert(t.sent=="pwd\r");
 t.received="directory\r\n";t.ended=true;session.tick(11,true,false,0,t,trust,input);assert(session.state.phase==SshPhase::Connected);session.tick(12,true,false,0,t,trust,input);assert(session.state.phase==SshPhase::Disconnected);
 for(SshTrust known:{SshTrust::Changed,SshTrust::Unavailable}){SshSession s;Transport remote;Trust keys;keys.known=known;s.begin(profile(),"secret",0);s.tick(1,true,false,0,remote,keys,input);s.tick(2,true,false,0,remote,keys,input);assert(s.state.phase==SshPhase::Failed && remote.authCalls==0);}
 SshSession reject;Transport remote;Trust keys;reject.begin(profile(),"secret",0);reject.tick(1,true,false,0,remote,keys,input);reject.tick(2,true,false,0,remote,keys,input);reject.tick(3,true,false,-1,remote,keys,input);assert(reject.state.phase==SshPhase::Disconnected && !remote.authCalls);
 keys.known=SshTrust::Match;SshSession auth;remote.authResult=SshIo::Denied;auth.begin(profile(),"secret",0);for(unsigned n=1;n<4;++n)auth.tick(n,true,false,0,remote,keys,input);assert(auth.state.phase==SshPhase::Failed && !strcmp(auth.state.message,"Authentication failed"));
 SshSession offline;offline.begin(profile(),"secret",0);offline.tick(1,false,false,0,remote,keys,input);assert(!strcmp(offline.state.message,"Wi-Fi unavailable"));
 SshSession timeout;remote.connectResult=SshIo::Again;timeout.begin(profile(),"secret",UINT32_MAX-99);timeout.tick(14900,true,false,0,remote,keys,input);assert(!strcmp(timeout.state.message,"SSH address setup timed out"));
 // A timeout identifies the actual blocked stage without changing its budget.
 for(auto stage:{SshPhase::Connecting,SshPhase::Handshake,SshPhase::Verify,SshPhase::Authenticating,SshPhase::Opening}){
  SshSession s;Transport peer;Trust saved;saved.known=SshTrust::Match;SshInputBuffer bytes;
  s.begin(profile(),"secret",0);
  if(stage!=SshPhase::Connecting)s.tick(1,true,false,0,peer,saved,bytes);
  if(stage==SshPhase::Verify){saved.known=SshTrust::Unknown;s.tick(2,true,false,0,peer,saved,bytes);}
  else if(stage==SshPhase::Authenticating || stage==SshPhase::Opening){s.tick(2,true,false,0,peer,saved,bytes);if(stage==SshPhase::Opening)s.tick(3,true,false,0,peer,saved,bytes);}
  assert(s.state.phase==stage);
  unsigned due=stage==SshPhase::Verify?120002:stage==SshPhase::Connecting?15000:stage==SshPhase::Handshake?15001:stage==SshPhase::Authenticating?15002:15003;
  s.tick(due,true,false,0,peer,saved,bytes);
  const char* expected=stage==SshPhase::Connecting?"SSH address setup timed out":stage==SshPhase::Handshake?"SSH negotiation timed out":stage==SshPhase::Verify?"Host verification timed out":stage==SshPhase::Authenticating?"SSH authentication timed out":"SSH shell setup timed out";
  assert(s.state.phase==SshPhase::Failed && !strcmp(s.state.message,expected));
 }
 SshSession cancel;cancel.begin(profile(),"secret",0);cancel.tick(1,true,true,0,remote,keys,input);assert(cancel.state.phase==SshPhase::Disconnected);
 // First-use saved acceptance also gates authentication until the next stage.
 SshSession accepted;Transport acceptedRemote;Trust newKeys;accepted.begin(profile(),"secret",0);
 accepted.tick(1,true,false,0,acceptedRemote,newKeys,input);accepted.tick(2,true,false,0,acceptedRemote,newKeys,input);
 accepted.tick(3,true,false,1,acceptedRemote,newKeys,input);assert(newKeys.saves==1 && acceptedRemote.authCalls==0);
 accepted.tick(4,true,false,0,acceptedRemote,newKeys,input);assert(acceptedRemote.authCalls==1);
 Transport unreachable;unreachable.connectResult=SshIo::Error;SshSession badHost;badHost.begin(profile(),"secret",0);badHost.tick(1,true,false,0,unreachable,keys,input);assert(badHost.state.phase==SshPhase::Failed && !strcmp(badHost.state.message,"Host unreachable"));
 Transport noFingerprint;noFingerprint.validFingerprint=false;SshSession invalidKey;invalidKey.begin(profile(),"secret",0);invalidKey.tick(1,true,false,0,noFingerprint,keys,input);invalidKey.tick(2,true,false,0,noFingerprint,keys,input);assert(invalidKey.state.phase==SshPhase::Failed && !noFingerprint.authCalls);
 Transport noPty;noPty.openResult=SshIo::Error;SshSession failedShell;failedShell.begin(profile(),"secret",0);for(unsigned n=1;n<=4;++n)failedShell.tick(n,true,false,0,noPty,keys,input);assert(failedShell.state.phase==SshPhase::Failed);
 Transport stalled;stalled.maxWrite=0;SshSession blocked;blocked.begin(profile(),"secret",0);for(unsigned n=1;n<=4;++n)blocked.tick(n,true,false,0,stalled,keys,input);assert(blocked.state.phase==SshPhase::Connected);assert(input.push("test",4));blocked.tick(5,true,false,0,stalled,keys,input);blocked.tick(10005,true,false,0,stalled,keys,input);assert(!strcmp(blocked.state.message,"SSH write timed out"));
 SshInputBuffer bounded;std::string full(512,'x');assert(bounded.push(full.data(),512));assert(!bounded.push("y",1));char data[512];assert(bounded.pop(data,511)==511);assert(bounded.push("y",1));assert(bounded.pop(data,512)==2 && data[0]=='x' && data[1]=='y');bounded.clear();assert(bounded.pop(data,1)==0);
}
