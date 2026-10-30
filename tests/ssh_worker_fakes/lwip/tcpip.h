#pragma once
#include "dns.h"
inline err_t tcpip_callback(void(*callback)(void*),void* context){if(sshfake::failure()==sshfake::DnsQueueFailed)return ERR_MEM;callback(context);return ERR_OK;}
