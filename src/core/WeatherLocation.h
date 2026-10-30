#pragma once
#include <stdint.h>
#include <math.h>
#include <string.h>
namespace spark {
enum class LocationMode : uint8_t { Auto, Home, Manual };
struct WeatherLocation {
    double latitude=40.7128,longitude=-74.0060;
    char label[28]="New York, NY",country[3]="US";
    bool approximate=false;
};
struct LocationConfig {
    uint32_t version=1;
    LocationMode mode=LocationMode::Auto;
    WeatherLocation home,manual,last;
};
inline bool validLocation(const WeatherLocation& l) {
    return isfinite(l.latitude) && isfinite(l.longitude) && l.latitude>=-90 && l.latitude<=90 &&
           l.longitude>=-180 && l.longitude<=180 && l.label[0] && l.label[27]==0 && l.country[2]==0;
}
inline bool sameLocation(const WeatherLocation& a,const WeatherLocation& b) {
    return fabs(a.latitude-b.latitude)<0.0001 && fabs(a.longitude-b.longitude)<0.0001;
}
}
