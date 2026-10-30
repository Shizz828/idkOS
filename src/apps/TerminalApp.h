#pragma once
#include "../core/Input.h"
#include "../terminal/SdShell.h"
#include "../ui/Screen.h"
namespace spark {
class TerminalApp {
    char command[TerminalCommandLimit+1]{},cwd[128]="/",deleteCommand[TerminalCommandLimit+1]{};
    char history[8][TerminalCommandLimit+1]{}; unsigned historyCount=0,historyPosition=0;
    char lines[TerminalLines][TerminalColumns+1]{}; uint16_t colors[TerminalLines]{};
    unsigned count=0,scroll=0; bool pending=false,confirm=false,timedOut=false,developer=false;
    uint32_t revision=0; TerminalResult result;
    void append(const char* value,uint16_t color=theme::Content) {
        if(count==TerminalLines) { memmove(lines,lines+1,sizeof(lines)-sizeof(lines[0])); memmove(colors,colors+1,sizeof(colors)-sizeof(colors[0]));--count; }
        snprintf(lines[count],sizeof(lines[0]),"%s",value); colors[count++]=color;scroll=0;
    }
    template<typename Storage> void submit(Storage& storage,const char* input,bool approved=false) {
        pending=developer?storage.submitSystemTerminal(input,cwd):storage.submitTerminal(input,cwd,approved);
        if(!pending) append("SD busy; retry",theme::Error);
        else timedOut=false;
    }
public:
    TerminalApp() {append("SD filesystem only. Type help.",theme::Primary);}
    bool setDeveloper(bool enabled) {if(pending)return false;developer=enabled;confirm=false;command[0]=0;strlcpy(cwd,"/",sizeof(cwd));append(enabled?"SYS:/ READ ONLY; type help":"SD:/ writable; type help",theme::Primary);return true;}
    bool cancel() {if(confirm){confirm=false;append("Deletion canceled",theme::Attention);return true;}return false;}
    template<typename Storage> bool poll(Storage& storage,uint32_t now) {
        if(!storage.terminalSnapshot(result)) return false;
        if(result.busy) {
            if(pending && !timedOut && uint32_t(now-result.started)>=15000) {timedOut=true;pending=false;append("SD timeout; G0 returns Home",theme::Error);return true;}
            return false;
        }
        if(result.revision==revision) return false;
        revision=result.revision;
        if(!pending && !timedOut) return false;
        pending=false;timedOut=false;strlcpy(cwd,result.cwd,sizeof(cwd));
        for(unsigned i=0;i<result.count;++i) append(result.lines[i],result.ok?(result.mutated?theme::Success:theme::Content):theme::Error);
        return true;
    }
    template<typename Storage> void input(const InputEvent& e,Storage& storage) {
        if(confirm) {
            if(e.enter || e.text[0]) {
                bool yes=!strcmp(e.text,"y")||!strcmp(e.text,"Y");confirm=false;
                if(yes) submit(storage,deleteCommand,true);else append("Deletion canceled",theme::Attention);
            }return;
        }
        if(e.left) {if(scroll+6<count) ++scroll;return;}
        if(e.right) {if(scroll) --scroll;return;}
        if(pending) return;
        if(e.up && historyCount) { if(historyPosition) --historyPosition; strlcpy(command,history[historyPosition],sizeof(command));return; }
        if(e.down && historyCount) {if(historyPosition<historyCount) ++historyPosition;strlcpy(command,historyPosition==historyCount?"":history[historyPosition],sizeof(command));return;}
        size_t n=strlen(command); if(e.erase && n) command[--n]=0;
        for(const char* c=e.text;*c && n<TerminalCommandLimit;++c){command[n++]=*c;command[n]=0;}
        if(!e.enter || !n) return;
        char echo[TerminalColumns+1];snprintf(echo,sizeof(echo),"> %.36s",command);append(echo,theme::Primary);
        if(historyCount==8){memmove(history,history+1,sizeof(history)-sizeof(history[0]));--historyCount;}
        strlcpy(history[historyCount++],command,sizeof(history[0]));historyPosition=historyCount;
        if(!strcmp(command,"clear")){count=scroll=0;command[0]=0;return;}
        char copy[TerminalCommandLimit+1];strlcpy(copy,command,sizeof(copy));char* args[24]{};int argc=shellTokens(copy,args,24);
        if(!developer && argc>0 && deleteVerb(args[0])) {
            strlcpy(deleteCommand,command,sizeof(deleteCommand));confirm=true;
            for(size_t i=0;i<strlen(deleteCommand);i+=TerminalColumns){char part[TerminalColumns+1];snprintf(part,sizeof(part),"%.38s",deleteCommand+i);append(part,theme::Error);}
            append("Execute deletion? [y/N]",theme::Attention);
        } else submit(storage,command);
        command[0]=0;
    }
    void draw() const {
        frame("LOCAL CONSOLE"); text(6,29,"ESP32 LOCAL CONSOLE",1,theme::Primary);
        unsigned end=count>scroll?count-scroll:0,start=end>6?end-6:0;
        for(unsigned i=start;i<end;++i) text(6,43+int(i-start)*11,lines[i],1,colors[i]);
        char prompt[39];snprintf(prompt,sizeof(prompt),"%s:%.30s%s",developer?"SYS":"SD",cwd,pending?" BUSY":confirm?" [y/N]":" >");
        text(6,110,prompt,1,confirm?theme::Attention:theme::Primary);
        char visible[39]{};size_t n=strlen(command);strlcpy(visible,command+(n>36?n-36:0),sizeof(visible));
        text(6,124,visible); int cursor=6+int(strlen(visible))*6;screen().drawFastHLine(cursor,132,5,theme::Primary);
    }
};
}
