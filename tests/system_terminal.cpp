#include <cassert>
#include "../src/terminal/SystemShell.h"
int main(){
    auto run=[](const char* command,const char* cwd="/"){spark::TerminalResult out;spark::executeSystem(command,cwd,out);assert(!out.mutated);return out;};
    assert(run("ls").count==2);assert(run("cat /info").ok);assert(run("ls /partitions").count==1);
    assert(run("cat /partitions/nvs").count==33);assert(!run("cat /partitions/missing").ok);
    assert(!strcmp(run("cd partitions").cwd,"/partitions"));assert(run("cat nvs","/partitions").ok);
    const char* forbidden[]={"rm /info","rm -r /partitions","rmdir /partitions","mkdir x","touch x","mv /info x","cp /info x","echo x > /info","echo x >> /info","format","reboot"};
    for(const char* command:forbidden)assert(!run(command).ok);
    assert(!run("cat ../../info","/partitions").ok);
}
