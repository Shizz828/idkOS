#pragma once
#include "State.h"
constexpr int WL_CONNECTED=3;
struct FakeWiFi {int status(){return sshfake::counts().online.load()?WL_CONNECTED:0;}};
static FakeWiFi WiFi;
