#include <cassert>
#include "../src/core/Navigation.h"
int main() {
    spark::Navigation n;
    n.move(-1); assert(n.selected==5);
    n.move(1); assert(n.selected==0);
    n.page(1); assert(n.selected==2);
    n.page(1); assert(n.selected==4);
    n.page(1); assert(n.selected==0);
    n.page(-1); assert(n.selected==4);
    n.open(); assert(n.active==4); n.home(); assert(n.active==-1);
    // Home launcher: Left/Right wrap over the five icons; SSH is not one of them.
    static_assert(spark::HomeAppCount==5,"five Home icons");
    n.selected=0;n.moveHome(-1);assert(n.selected==4);n.moveHome(1);assert(n.selected==0);
    for(int i=0;i<4;++i)n.moveHome(1);
    assert(n.selected==4);
    n.selected=5;n.moveHome(1);assert(n.selected==0); // After Fn+S.
    n.selected=5;n.moveHome(-1);assert(n.selected==4);
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
