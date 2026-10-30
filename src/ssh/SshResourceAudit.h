#pragma once
#include <atomic>
namespace spark {
struct SshResourceAudit {
    std::atomic<unsigned> sessions{0},channels{0},sockets{0};
};
inline SshResourceAudit& sshResources(){static SshResourceAudit counters;return counters;}
}
