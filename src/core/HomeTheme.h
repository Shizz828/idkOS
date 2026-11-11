#pragma once
#include <stdint.h>
namespace spark {
// Home screen look only: every theme uses the same launcher, order and keys.
// Saved as one byte in its own NVS namespace "spark-home", key "theme" (no other
// code uses that namespace). A missing or unknown value means Subway.
enum class HomeTheme : uint8_t { Subway=0, MetroMinimal=1, Classic=2 };
constexpr unsigned HomeThemeCount=3;
constexpr const char* HomeThemeNamespace="spark-home";
inline const char* homeThemeName(HomeTheme theme){
    switch(theme){case HomeTheme::MetroMinimal:return "Metro Minimal";case HomeTheme::Classic:return "Classic V1";default:return "Subway";}
}
inline const char* homeThemeAbout(HomeTheme theme){
    switch(theme){case HomeTheme::MetroMinimal:return "Clean, modern look";case HomeTheme::Classic:return "Original idkOS look";default:return "Cyan subway style";}
}
inline HomeTheme homeThemeFromStored(uint8_t value){return value<HomeThemeCount?HomeTheme(value):HomeTheme::Subway;}
// Kept in RAM, so screen sleep, pocket sleep and wake never change it.
inline HomeTheme& homeTheme(){static HomeTheme theme=HomeTheme::Subway;return theme;}
template<typename Store> void loadHomeTheme(Store& store){homeTheme()=homeThemeFromStored(store.getUChar("theme",0));}
// Applies the theme even when the save fails, so the change is visible right away.
template<typename Store> bool saveHomeTheme(Store& store,HomeTheme theme){
    homeTheme()=theme;return store.putUChar("theme",uint8_t(theme))==1;
}
}
