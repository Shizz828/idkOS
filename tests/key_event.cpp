#include <cassert>
#include <deque>
#include <vector>
#include "../src/core/KeyEvent.h"
struct Point { int x=0,y=0;bool operator==(const Point& o) const {return x==o.x && y==o.y;} };
// The chip as the driver sees it: a ten event FIFO, and IRQ low until the
// interrupt is acknowledged with the FIFO empty.
struct Chip {
    std::deque<uint8_t> fifo;bool interrupt=false;unsigned accesses=0;
    void event(uint8_t e){if(fifo.size()<10)fifo.push_back(e);interrupt=true;}
    bool irqLow() const {return interrupt;}
    uint8_t getEvent(){++accesses;if(fifo.empty())return 0;const uint8_t e=fifo.front();fifo.pop_front();return e;}
    void writeRegister8(uint8_t reg,uint8_t value){++accesses;assert(reg==spark::KeyInterruptStatus && value==spark::KeyEventInterrupt);if(fifo.empty())interrupt=false;}
};
static uint8_t code(int x,int y,bool down){return uint8_t((x/2*10+y+(x&1?4:0)+1)|(down?0x80:0));}
static bool held(const std::vector<Point>& keys,int x,int y){Point p;p.x=x;p.y=y;return std::find(keys.begin(),keys.end(),p)!=keys.end();}
int main() {
    using spark::readKeyEvent;
    const int spaceX=13,ctrlX=0,row=3;
    { // Every matrix position decodes as the library's reader does: Space, Ctrl and the arrows.
        for(int y=0;y<4;++y)for(int x=0;x<14;++x) {
            Chip c;std::vector<Point> keys;c.event(code(x,y,true));readKeyEvent(c,c.irqLow(),keys);
            assert(keys.size()==1 && keys[0].x==x && keys[0].y==y);
        }
    }
    { // Idle: no I2C traffic at all.
        Chip c;std::vector<Point> keys;for(int i=0;i<100;++i)readKeyEvent(c,c.irqLow(),keys);assert(!c.accesses);
    }
    { // Two keys together: one event per pass, as before, in order.
        Chip c;std::vector<Point> keys;c.event(code(spaceX,row,true));c.event(code(ctrlX,row,true));
        readKeyEvent(c,c.irqLow(),keys);assert(keys.size()==1 && c.irqLow());
        readKeyEvent(c,c.irqLow(),keys);assert(keys.size()==2 && !c.irqLow());
    }
    { // The lost interrupt: an event lands right after the acknowledgement that
      // emptied the FIFO. IRQ is low with no new edge, and six more events queue up.
      // Reading follows the level, so everything arrives and nothing stays held.
        Chip c;std::vector<Point> keys;
        c.event(code(spaceX,row,true));c.event(code(ctrlX,row,true));
        readKeyEvent(c,c.irqLow(),keys);readKeyEvent(c,c.irqLow(),keys);
        c.event(code(spaceX,row,false));c.event(code(ctrlX,row,false));
        for(int i=0;i<3;++i){c.event(code(spaceX,row,true));c.event(code(spaceX,row,false));}
        assert(c.irqLow() && c.fifo.size()==8);
        for(int pass=0;pass<8;++pass)readKeyEvent(c,c.irqLow(),keys);
        assert(keys.empty() && !c.irqLow() && c.fifo.empty());
    }
    { // A late press is not lost, and a held key stays held until its release.
        Chip c;std::vector<Point> keys;c.event(code(10,row,true));readKeyEvent(c,c.irqLow(),keys);
        for(int i=0;i<1000;++i)readKeyEvent(c,c.irqLow(),keys);
        assert(held(keys,10,row));
        c.event(code(10,row,false));readKeyEvent(c,c.irqLow(),keys);assert(keys.empty());
    }
    { // IRQ low with an empty FIFO is acknowledged; GPIO event codes are not keys.
        Chip c;std::vector<Point> keys;c.interrupt=true;readKeyEvent(c,c.irqLow(),keys);assert(!c.irqLow() && keys.empty());
        c.event(0x62);c.event(0xE2);readKeyEvent(c,c.irqLow(),keys);readKeyEvent(c,c.irqLow(),keys);assert(!c.irqLow() && keys.empty());
    }
}
