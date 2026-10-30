#pragma once
#include <atomic>
#include <mutex>
#include <vector>
#include <string>
#include <cassert>
namespace sshfake {
enum Failure {None,DnsFailed,DnsQueueFailed,DnsDelayed,SessionAllocation,OptionSetup,Handshake,Authentication,ChannelAllocation,ShellSetup};
inline std::atomic<Failure>& failure(){static std::atomic<Failure> value{None};return value;}
struct Counters {
 std::atomic<unsigned> sessions{0},channels{0},sockets{0},keys{0},hashes{0},initializations{0},tasks{0},stackMargin{24000};
 std::atomic<bool> online{true},remoteExit{false},readFailure{false};
 unsigned heap()const{return 160000-(initializations.load()?4096:0)-sessions.load()*2048-channels.load()*1024-sockets.load()*512-keys.load()*64-hashes.load()*32;}
};
inline Counters& counts(){static Counters value;return value;}
inline std::mutex& logMutex(){static std::mutex value;return value;}
inline std::vector<std::string>& logs(){static std::vector<std::string> value;return value;}
}
