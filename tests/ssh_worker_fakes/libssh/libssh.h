#pragma once
// Minimal API model for fault-injecting the production transport/service. The
// real ESP32 build independently checks these calls against the pinned library.
#include "../State.h"
#include <cstring>
#include <cstdint>
#include <cstdlib>
constexpr int SSH_OK=0,SSH_ERROR=-1,SSH_AGAIN=-2,SSH_INVALID_SOCKET=-1;
constexpr int SSH_AUTH_SUCCESS=0,SSH_AUTH_DENIED=1,SSH_AUTH_PARTIAL=2,SSH_AUTH_AGAIN=4,SSH_LOG_NOLOG=0;
enum ssh_options_e {SSH_OPTIONS_HOST,SSH_OPTIONS_PORT,SSH_OPTIONS_USER,SSH_OPTIONS_TIMEOUT,SSH_OPTIONS_LOG_VERBOSITY,SSH_OPTIONS_PROCESS_CONFIG};
enum ssh_publickey_hash_type {SSH_PUBLICKEY_HASH_SHA256};
struct FakeChannel;
struct FakeSession {int fd=-1;FakeChannel* channel=nullptr;};
struct FakeChannel {FakeSession* session;};
using ssh_session=FakeSession*;using ssh_channel=FakeChannel*;using ssh_key=int*;
inline ssh_session ssh_new(){if(sshfake::failure()==sshfake::SessionAllocation)return nullptr;++sshfake::counts().sessions;return new FakeSession;}
inline int ssh_options_set(ssh_session,ssh_options_e option,const void* value){if(option==SSH_OPTIONS_HOST)assert(!strcmp(static_cast<const char*>(value),"127.0.0.1"));return sshfake::failure()==sshfake::OptionSetup?SSH_ERROR:SSH_OK;}
inline void ssh_set_blocking(ssh_session,int value){assert(value==0);}
inline void fakeCloseSocket(ssh_session session){if(session->fd>=0){session->fd=-1;--sshfake::counts().sockets;}}
inline int ssh_connect(ssh_session session){if(session->fd<0){session->fd=42;++sshfake::counts().sockets;}if(sshfake::failure()==sshfake::Handshake){fakeCloseSocket(session);return SSH_ERROR;}return SSH_OK;}
inline int ssh_get_fd(ssh_session session){return session->fd;}
inline const char* ssh_get_error(void*){return "Injected SSH failure";}
inline void ssh_disconnect(ssh_session session){fakeCloseSocket(session);}
inline void ssh_free(ssh_session session){fakeCloseSocket(session);if(session->channel){delete session->channel;--sshfake::counts().channels;}delete session;--sshfake::counts().sessions;}
inline int ssh_get_server_publickey(ssh_session,ssh_key* key){*key=new int;++sshfake::counts().keys;return SSH_OK;}
inline int ssh_get_publickey_hash(ssh_key,ssh_publickey_hash_type,unsigned char** hash,size_t* size){*size=32;*hash=new unsigned char[32]{};++sshfake::counts().hashes;return SSH_OK;}
inline void ssh_key_free(ssh_key key){delete key;--sshfake::counts().keys;}
inline void ssh_clean_pubkey_hash(unsigned char** hash){delete[] *hash;*hash=nullptr;--sshfake::counts().hashes;}
inline int ssh_userauth_password(ssh_session,const char*,const char* password){assert(!strcmp(password,"test-secret"));return sshfake::failure()==sshfake::Authentication?SSH_AUTH_DENIED:SSH_AUTH_SUCCESS;}
inline ssh_channel ssh_channel_new(ssh_session session){if(sshfake::failure()==sshfake::ChannelAllocation)return nullptr;auto channel=new FakeChannel{session};session->channel=channel;++sshfake::counts().channels;return channel;}
inline int ssh_channel_open_session(ssh_channel){return SSH_OK;}
inline int ssh_channel_request_pty_size(ssh_channel,const char*,int,int){return sshfake::failure()==sshfake::ShellSetup?SSH_ERROR:SSH_OK;}
inline int ssh_channel_request_shell(ssh_channel){return SSH_OK;}
inline int ssh_channel_write(ssh_channel,const void*,uint32_t size){return int(size);}
inline int ssh_channel_read_nonblocking(ssh_channel channel,void*,uint32_t,int){if(sshfake::counts().readFailure){fakeCloseSocket(channel->session);return SSH_ERROR;}return 0;}
inline int ssh_is_connected(ssh_session session){return session->fd>=0;}
inline int ssh_channel_is_eof(ssh_channel){return sshfake::counts().remoteExit;}
inline int ssh_channel_is_closed(ssh_channel){return 0;}
inline int ssh_channel_send_eof(ssh_channel){return SSH_OK;}
inline int ssh_channel_close(ssh_channel){return SSH_OK;}
inline void ssh_channel_free(ssh_channel){/* Model libssh's deferred channel free, completed by ssh_free. */}
