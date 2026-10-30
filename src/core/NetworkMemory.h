#pragma once
#include <atomic>
namespace spark {
// Serialize allocation-heavy HTTPS jobs and SSH startup. A pending SSH request
// gets the next turn, without interrupting a request already in progress.
class NetworkMemoryGate {
    std::atomic<unsigned> owner{0}; // 0 idle, 1 HTTPS, 2 SSH startup
    std::atomic<bool> pending{false};
public:
    void requestSsh(){pending.store(true);}
    bool trySsh(){unsigned idle=0;return owner.compare_exchange_strong(idle,2);}
    void releaseSsh(){owner.store(0);pending.store(false);}
    void cancelSsh(){pending.store(false);}
    bool tryHttp(){
        if(pending.load())return false;
        unsigned idle=0;if(!owner.compare_exchange_strong(idle,1))return false;
        if(pending.load()){owner.store(0);return false;}
        return true;
    }
    void releaseHttp(){owner.store(0);}
    bool httpActive()const{return owner.load()==1;}
};
inline NetworkMemoryGate& networkMemory(){static NetworkMemoryGate gate;return gate;}
class HttpMemoryLease {
    NetworkMemoryGate& gate;bool acquired;
public:
    explicit HttpMemoryLease(NetworkMemoryGate& memory=networkMemory()):gate(memory),acquired(gate.tryHttp()){}
    ~HttpMemoryLease(){if(acquired)gate.releaseHttp();}
    explicit operator bool()const{return acquired;}
    HttpMemoryLease(const HttpMemoryLease&)=delete;
    HttpMemoryLease& operator=(const HttpMemoryLease&)=delete;
};
}
