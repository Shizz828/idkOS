#pragma once
constexpr int SOL_SOCKET=1,SO_KEEPALIVE=9,IPPROTO_TCP=6,TCP_KEEPIDLE=4,TCP_KEEPINTVL=5,TCP_KEEPCNT=6;
inline int setsockopt(int,int,int,const void*,unsigned){return 0;}
