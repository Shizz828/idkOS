#pragma once
#include "State.h"
constexpr unsigned MALLOC_CAP_INTERNAL=1,MALLOC_CAP_8BIT=2;
inline unsigned heap_caps_get_free_size(unsigned){return sshfake::counts().heap();}
inline unsigned heap_caps_get_largest_free_block(unsigned){return sshfake::counts().heap()/2;}
inline unsigned heap_caps_get_minimum_free_size(unsigned){return 64000;}
