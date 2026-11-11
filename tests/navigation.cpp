#include <cassert>
#include "../src/core/Navigation.h"
#include "../src/core/AppRegistry.h"
int main() {
    spark::Navigation n;
    n.move(-1); assert(n.selected==6);
    n.move(1); assert(n.selected==0);
    n.page(1); assert(n.selected==2);
    n.page(1); assert(n.selected==4);
    n.page(1); assert(n.selected==6);
    n.page(1); assert(n.selected==0);
    n.page(-1); assert(n.selected==6);
    n.selected=4; n.open(); assert(n.active==4); n.home(); assert(n.active==-1);
    // Home launcher: Left/Right wrap over six icons in HomeOrder - Cache, Weather,
    // Crypto, Terminal, Calendar, Settings (Settings last). SSH is not one of them.
    static_assert(spark::HomeAppCount==6,"six Home icons");
    const unsigned order[]={spark::CaptureIndex,spark::WeatherIndex,spark::CryptoIndex,spark::ConsoleIndex,spark::CalendarIndex,spark::SettingsIndex};
    for(unsigned i=0;i<6;++i) assert(spark::HomeOrder[i]==order[i] && spark::homePosition(order[i])==int(i));
    assert(spark::homePosition(spark::SshIndex)==-1);
    n.selected=0;n.moveHome(-1);assert(n.selected==spark::SettingsIndex);n.moveHome(1);assert(n.selected==0);
    for(int i=0;i<4;++i)n.moveHome(1);
    assert(n.selected==spark::CalendarIndex);   // Calendar's index (6) is past the Home count.
    n.moveHome(1);assert(n.selected==spark::SettingsIndex);
    n.moveHome(-1);assert(n.selected==spark::CalendarIndex);
    n.selected=spark::SshIndex;n.moveHome(1);assert(n.selected==0); // After Fn+S.
    n.selected=spark::SshIndex;n.moveHome(-1);assert(n.selected==spark::SettingsIndex);
    n.selected=2;n.open();assert(n.active==2);n.home();assert(n.selected==2); // Highlight kept.
    spark::RepeatKey r;
    assert(r.update(true,0)); assert(!r.update(true,399));
    assert(r.update(true,400)); assert(!r.update(true,479));
    assert(r.update(true,480)); assert(!r.update(false,481));
    assert(r.update(true,482));
    spark::RepeatKey wrap;
    assert(wrap.update(true,0xffffff00));
    assert(!wrap.update(true,0x8f)); assert(wrap.update(true,0x90));
}
