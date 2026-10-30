#pragma once
#include "../State.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
struct ip_addr_t {uint32_t addr=0;};
using ip4_addr_t=ip_addr_t;
using err_t=int;
constexpr err_t ERR_OK=0,ERR_INPROGRESS=-5,ERR_MEM=-1;
#define IP_IS_V4(addr) (true)
#define ip_2_ip4(addr) (addr)
using DnsCallback=void(*)(const char*,const ip_addr_t*,void*);
struct PendingDns {DnsCallback callback;void* context;};
inline std::vector<PendingDns>& pendingDns(){static std::vector<PendingDns> list;return list;}
inline err_t dns_gethostbyname(const char*,ip_addr_t* address,DnsCallback callback,void* context){
 if(sshfake::failure()==sshfake::DnsFailed)return ERR_MEM;
 if(sshfake::failure()==sshfake::DnsDelayed){pendingDns().push_back({callback,context});return ERR_INPROGRESS;}
 address->addr=1;return ERR_OK;
}
inline int ip4addr_aton(const char* host,ip4_addr_t* address){if(strcmp(host,"127.0.0.1"))return 0;address->addr=1;return 1;}
inline char* ip4addr_ntoa_r(const ip4_addr_t*,char* out,int size){snprintf(out,size,"127.0.0.1");return out;}
inline void completeDns(){auto old=pendingDns();pendingDns().clear();ip_addr_t address;address.addr=1;for(auto entry:old)entry.callback(nullptr,&address,entry.context);}
