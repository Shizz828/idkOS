#pragma once
#include <stdint.h>
namespace spark { namespace theme {
constexpr uint16_t Primary=0x07FF,Success=0x07E0,Error=0xF800,Attention=0xFFE0,Content=0xFFFF,Background=0;
// Route colors identify Home apps independently of status/error colors.
constexpr uint16_t RouteCapture=0xF800,RouteWeather=0x041F,RouteCrypto=0x07E0,
    RouteTerminal=0xFFE0,RouteSettings=0xC618;
} }
