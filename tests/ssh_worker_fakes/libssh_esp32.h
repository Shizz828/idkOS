#pragma once
#include "State.h"
inline void libssh_begin(){sshfake::counts().initializations.fetch_add(1);}
