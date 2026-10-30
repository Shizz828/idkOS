#include <cassert>
#include <string>
#include "../src/ssh/SshPersistence.h"
int main(){
 Preferences::data().clear();spark::SshProfiles profiles;assert(profiles.begin());assert(!profiles.count());
 spark::SshProfile p;strcpy(p.name,"Linux");strcpy(p.host,"SERVER.Example");strcpy(p.user,"student");p.port=2222;
 assert(profiles.put(p));spark::SshProfiles restored;assert(restored.begin() && restored.count()==1);assert(!strcmp(restored.at(0).host,"server.example") && restored.at(0).port==2222);
 for(unsigned i=1;i<5;++i){snprintf(p.name,sizeof(p.name),"Host %u",i);assert(profiles.put(p));}strcpy(p.name,"Overflow");assert(!profiles.put(p));assert(restored.erase(0));
 uint16_t port=0;assert(spark::sshPort("22",port)&&port==22);assert(spark::sshPort("65535",port)&&port==65535);
 for(const char* value:{"","0","65536","-1","22x","9999999999999999"})assert(!spark::sshPort(value,port));
 strcpy(p.host,"server/command");assert(!profiles.put(p));strcpy(p.host,"server.example");
 Preferences::writesFail()=true;assert(!restored.put(p));assert(!restored.erase(0));Preferences::writesFail()=false;
 spark::SshKnownHosts hosts;hosts.begin();std::string fingerprint="SHA256:"+std::string(43,'A'),other="SHA256:"+std::string(43,'B');
 assert(hosts.check(p,fingerprint.c_str())==spark::SshTrust::Unknown);assert(hosts.remember(p,fingerprint.c_str()));
 spark::SshKnownHosts saved;saved.begin();assert(saved.check(p,fingerprint.c_str())==spark::SshTrust::Match);assert(saved.check(p,other.c_str())==spark::SshTrust::Changed);assert(!saved.remember(p,other.c_str()));
 p.port=22;assert(saved.check(p,fingerprint.c_str())==spark::SshTrust::Unknown);
 Preferences::writesFail()=true;assert(!saved.remember(p,fingerprint.c_str()));assert(saved.check(p,fingerprint.c_str())==spark::SshTrust::Unknown);Preferences::writesFail()=false;
 auto& keys=Preferences::data()["spark-ssh-keys/hosts"];keys[0]^=1;spark::SshKnownHosts corrupt;corrupt.begin();assert(corrupt.check(p,fingerprint.c_str())==spark::SshTrust::Unavailable);
 auto& bytes=Preferences::data()["spark-ssh/profiles"];bytes[0]^=1;spark::SshProfiles damaged;assert(!damaged.begin() && damaged.count()==0);
}
