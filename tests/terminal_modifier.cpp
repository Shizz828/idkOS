#include <cassert>
#include <string>
#include "../src/apps/TerminalApp.h"
#include "../src/apps/SshApp.h"
struct Storage {
    std::string command;
    bool submitTerminal(const char* value,const char*,bool){command=value;return true;}
    bool submitSystemTerminal(const char* value,const char*){command=value;return true;}
};
struct Service {
    spark::SshSnapshot state;std::string sent;int scrolled=0;
    bool snapshot(spark::SshSnapshot& out){out=state;return true;}
    bool busy(){return false;}const char* startError(){return "";}
    bool connect(const spark::SshProfile& profile,const char*){state.profile=profile;state.phase=spark::SshPhase::Connecting;++state.revision;return true;}
    bool send(const char* data,unsigned size){sent.append(data,size);return true;}
    void disconnect(){}void verify(int){}void scroll(int count){scrolled+=count;}
};
spark::InputEvent text(const char* value){spark::InputEvent e;strcpy(e.text,value);return e;}
int main(){
    using namespace spark;
    auto& keyboard=M5Cardputer.Keyboard;
    Input localInput;TerminalApp local;Storage storage;
    local.input(text("cd "),storage);
    keyboard.s.shift=true;keyboard.keys={{0,0}};
    auto e=localInput.poll(100,true);assert(!e.back && !strcmp(e.text,"~"));local.input(e,storage);
    keyboard.keys.clear();keyboard.s.shift=false;localInput.poll(101,true);
    keyboard.keys={{13,2}};e=localInput.poll(102,true);local.input(e,storage);
    assert(storage.command=="cd ~");
    // Connect a fake SSH transport, then feed the same shared physical chord.
    Preferences::data().clear();SshApp ssh;Service remote;ssh.open(remote);
    InputEvent enter;enter.enter=true;
    ssh.input(text("host.example"),remote,true);ssh.input(enter,remote,true);ssh.input(enter,remote,true);
    ssh.input(text("user"),remote,true);ssh.input(enter,remote,true);
    ssh.input(text("password"),remote,true);ssh.input(enter,remote,true);
    remote.state.phase=SshPhase::Connected;++remote.state.revision;ssh.poll(remote);
    Input sshInput;ssh.input(text("cd "),remote,true);
    keyboard.s.shift=true;keyboard.keys={{0,0}};e=sshInput.poll(200,true);
    assert(!e.back && !e.sleep && !strcmp(e.text,"~"));ssh.input(e,remote,true);
    keyboard.keys.clear();keyboard.s.shift=false;sshInput.poll(201,true);
    keyboard.keys={{13,2}};e=sshInput.poll(202,true);ssh.input(e,remote,true);
    assert(remote.sent=="cd ~\r");
    keyboard.keys.clear();sshInput.poll(203,true);
    int previousScroll=remote.scrolled;
    keyboard.s.shift=keyboard.s.fn=true;keyboard.keys={{11,2}};
    e=sshInput.poll(204,true);ssh.input(e,remote,true);
    assert(remote.scrolled==previousScroll+3 && remote.sent=="cd ~\r");
    keyboard.keys.clear();sshInput.poll(205,true);
    keyboard.s.shift=false;keyboard.keys={{0,0}};e=sshInput.poll(206,true);
    assert(e.sleep && !e.back && !e.text[0]);
    keyboard.keys.clear();keyboard.s.fn=false;sshInput.poll(207,true);
    keyboard.keys={{0,0}};assert(!sshInput.poll(208,true).back);
    e=sshInput.poll(243,true);assert(e.back && !e.sleep && !e.text[0]);
}
