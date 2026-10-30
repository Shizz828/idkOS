#include <cassert>
#include <string>
#include "../src/ssh/SshTerminal.h"
int main(){
 spark::SshTerminal t;spark::SshSnapshot s;
 auto feed=[&](const char* value){t.feed(value,strlen(value));};
 std::string first="user@host$ ";t.feed(first.data(),first.size());t.snapshot(s);assert(!strncmp(s.rows[0],"user@host$ ",11));assert(s.cursorX==11 && s.cursorY==0);
 feed("abc\bD\r\n");t.snapshot(s);assert(!strncmp(s.rows[0],"user@host$ abD",14));assert(s.cursorX==0 && s.cursorY==1);
 feed("\033[31mred\033[0m\033]0;private title\007text");t.snapshot(s);assert(!strncmp(s.rows[1],"redtext",7));
 feed("\033[2J\033[H");std::string longLine(38,'X');t.feed(longLine.data(),longLine.size());feed("\r\nY");t.snapshot(s);assert(s.cursorY==1 && s.rows[1][0]=='Y'); // No extra blank row after CRLF at margin.
 feed("\033[2;3Habc\033[2D!\033[K");t.snapshot(s);assert(s.rows[1][2]=='a' && s.rows[1][3]=='!' && s.rows[1][4]==' ');
 for(int i=0;i<100;++i){char line[30];snprintf(line,sizeof(line),"\r\nline %03d",i);t.feed(line,strlen(line));}
 t.scroll(999);t.snapshot(s);assert(s.scroll==48);assert(strstr(s.rows[0],"line 045"));
 t.scroll(-999);t.snapshot(s);assert(!s.scroll && strstr(s.rows[6],"line 099"));
 // Escape/control sequences can be split across packets and cursor values clamp.
 feed("\033[999999999999999");feed(";99999H");feed("Z");t.snapshot(s);assert(s.rows[6][37]=='Z');
 feed("\033]ignored");feed("\033\\");feed("\rA");t.snapshot(s);assert(s.rows[6][0]=='A');
 // Arbitrary, fragmented remote bytes cannot move the cursor out of bounds.
 spark::SshTerminal hostile;uint32_t random=12345;
 for(unsigned i=0;i<50000;++i){random=random*1664525u+1013904223u;char c=char(random>>24);hostile.feed(&c,1);if(i%97==0){hostile.scroll(i%2?100:-100);hostile.snapshot(s);assert(s.cursorX<spark::SshColumns && s.cursorY<spark::SshRows);for(const auto& row:s.rows)assert(row[spark::SshColumns]==0);}}
}
