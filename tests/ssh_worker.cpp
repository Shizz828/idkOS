#include <cassert>
#include <functional>
#include <thread>
#include "../src/services/SshService.cpp"
using namespace spark;
using namespace sshfake;
void waitUntil(const std::function<bool()>& condition){for(unsigned i=0;i<5000;++i){if(condition())return;std::this_thread::sleep_for(std::chrono::milliseconds(1));}assert(!"worker wait timed out");}
SshSnapshot snapshot(SshService& service){SshSnapshot state;while(!service.snapshot(state))std::this_thread::yield();return state;}
void clean(SshService& service,unsigned baseline){
 waitUntil([&]{return !service.busy();});
 assert(!counts().sessions && !counts().channels && !counts().sockets && !counts().keys && !counts().hashes);
 assert(!sshResources().sessions && !sshResources().channels && !sshResources().sockets);
 assert(counts().heap()==baseline);assert(counts().tasks==1 && counts().initializations==1);
 assert(networkMemory().tryHttp());networkMemory().releaseHttp();
}
int main(){
 Preferences::data().clear();SshService service;assert(service.begin());assert(service.begin());
 waitUntil([]{return counts().initializations==1;});unsigned baseline=counts().heap();
 SshProfile p;strcpy(p.host,"host.example");strcpy(p.user,"test");
 // Exercise the production service, transport and session with library/RTOS
 // fault injection. Allocation counters model the owned library resources;
 // hardware heap/coalescing is separately checked via the serial stage logs.
 for(unsigned cycle=0;cycle<200;++cycle){
  counts().remoteExit=false;counts().readFailure=false;counts().online=true;
  Failure cases[]={None,DnsFailed,DnsQueueFailed,SessionAllocation,OptionSetup,Handshake,Authentication,ChannelAllocation,ShellSetup,None};
  failure()=cases[cycle%10];assert(service.connect(p,"test-secret"));
  bool verified=false,connected=false;
  waitUntil([&]{
   auto state=snapshot(service);
   if(state.phase==SshPhase::Verify && !verified){service.verify(cycle%40==0?-1:2);verified=true;}
   if(state.phase==SshPhase::Connected && !connected){
    connected=true;
    assert(service.send("pwd\r",4));
    if(cycle%4==0)counts().remoteExit=true;
    else if(cycle%4==1)service.disconnect();
    else if(cycle%4==2)counts().online=false;
    else counts().readFailure=true;
   }
   return !service.busy();
  });
  clean(service,baseline);assert(SshDnsRequest::pending()==0);
 }
 counts().online=true;counts().remoteExit=false;counts().readFailure=false;
 // Cancellation leaves at most two fixed late-DNS slots, not an allocated
 // request per retry. Late completions release the right slot safely.
 for(unsigned round=0;round<20;++round){
  failure()=DnsDelayed;
  for(unsigned i=0;i<2;++i){assert(service.connect(p,"test-secret"));waitUntil([&]{return SshDnsRequest::pending()==i+1;});service.disconnect();clean(service,baseline);}
  assert(service.connect(p,"test-secret"));clean(service,baseline);assert(snapshot(service).phase==SshPhase::Failed);assert(SshDnsRequest::pending()==2);
  // A literal IP bypasses DNS even while both old callback slots are busy.
  SshProfile numeric=p;strcpy(numeric.host,"127.0.0.1");assert(service.connect(numeric,"test-secret"));
  waitUntil([&]{auto state=snapshot(service);if(state.phase==SshPhase::Verify)service.verify(2);return state.phase==SshPhase::Connected;});service.disconnect();clean(service,baseline);
  completeDns();assert(!SshDnsRequest::pending());
 }
 // An in-flight HTTPS job finishes before SSH allocates handshake state.
 failure()=None;assert(networkMemory().tryHttp());assert(service.connect(p,"test-secret"));
 waitUntil([&]{return !strcmp(snapshot(service).message,"Waiting for network RAM");});
 assert(!counts().sessions);assert(!networkMemory().tryHttp());service.disconnect();waitUntil([&]{return !service.busy();});networkMemory().releaseHttp();clean(service,baseline);
 // The private password must survive waiting for the HTTPS lease.
 assert(networkMemory().tryHttp());assert(service.connect(p,"test-secret"));waitUntil([&]{return !strcmp(snapshot(service).message,"Waiting for network RAM");});networkMemory().releaseHttp();
 waitUntil([&]{auto state=snapshot(service);if(state.phase==SshPhase::Verify)service.verify(2);return state.phase==SshPhase::Connected;});service.disconnect();clean(service,baseline);
 // Graceful stack-margin stop also follows the same complete cleanup path.
 counts().stackMargin=1024;assert(service.connect(p,"test-secret"));clean(service,baseline);assert(snapshot(service).phase==SshPhase::Failed);counts().stackMargin=24000;
 // Let the reusable worker report the settled baseline, after all destructors.
 waitUntil([]{std::lock_guard<std::mutex> lock(logMutex());for(const auto& row:logs())if(row.find("settled-idle")!=std::string::npos)return true;return false;});
 {std::lock_guard<std::mutex> lock(logMutex());bool failed=false,disconnected=false;unsigned starts=0;for(const auto& row:logs()){assert(row.find("test-secret")==std::string::npos);starts+=row.find("before-start")!=std::string::npos;failed|=row.find("after-failed-cleanup")!=std::string::npos;disconnected|=row.find("after-disconnect-cleanup")!=std::string::npos;}assert(failed && disconnected && starts==283);}
 auto task=fakeWorker();{std::lock_guard<std::mutex> lock(task->mutex);task->stop=true;task->cv.notify_all();}task->thread.join();
}
