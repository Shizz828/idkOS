#pragma once
#include "TerminalData.h"
#include <ctype.h>
#include <strings.h>
#include <stdlib.h>
namespace spark {
inline bool sdPath(const char* cwd,const char* input,char* result,size_t capacity) {
    if(input && !strncmp(input,"SD:/",4))input+=3;
    if(!input || !*input || strlen(input)>=128) return false;
    char combined[256]; snprintf(combined,sizeof(combined),"%s%s%s",input[0]=='/'?"":cwd,input[0]=='/' || !strcmp(cwd,"/")?"":"/",input);
    result[0]='/'; result[1]=0; size_t length=1;
    char* cursor=combined;
    while(*cursor) {
        while(*cursor=='/') ++cursor;
        if(!*cursor) break;
        char* component=cursor; while(*cursor && *cursor!='/') ++cursor;
        if(*cursor) *cursor++=0;
        if(!strcmp(component,".")) continue;
        if(!strcmp(component,"..")) {
            if(length==1) return false;
            while(length>1 && result[length-1]!='/') --length;
            if(length>1) --length;
            result[length]=0; continue;
        }
        for(const char* p=component;*p;++p) if(uint8_t(*p)<32 || *p=='\\' || *p==':' || *p=='*' || *p=='?') return false;
        size_t n=strlen(component); if(length+n+(length>1?1:0)>=capacity) return false;
        if(length>1) result[length++]='/';
        memcpy(result+length,component,n+1); length+=n;
    }
    return true;
}
inline bool managedNote(const char* path) {
    if(strlen(path)!=33 || strncasecmp(path,"/idkos/notes/",13) || strcasecmp(path+29,".txt")) return false;
    for(unsigned i=13;i<29;++i) if(!isxdigit(static_cast<unsigned char>(path[i]))) return false;
    return true;
}
template<typename Filesystem>
bool noteRecoveryCleanup(Filesystem& fs,const char* path,bool metadata) {
    if(!managedNote(path)) return true;
    char related[40];
    const char* extensions[]={"bak","tmp","rem","rbak","rtmp"};
    for(unsigned i=0;i<(metadata?5u:2u);++i) {
        snprintf(related,sizeof(related),"%.29s.%s",path,extensions[i]);
        if(fs.exists(related) && !fs.remove(related)) return false;
    }
    return true;
}
// Bounded argument parser; quotes support spaces. No executable expansion.
inline int shellTokens(char* input,char** tokens,unsigned limit=3) {
    unsigned count=0; char* read=input; char* write=input;
    while(*read) {
        while(*read==' ') ++read;
        if(!*read) break;
        if(count==limit) return -1;
        tokens[count++]=write; char quote=0;
        while(*read && (quote || *read!=' ')) {
            char c=*read++;
            if(c=='"' || c=='\'') { if(!quote) quote=c; else if(quote==c) quote=0; else *write++=c; }
            else *write++=c;
        }
        if(quote) return -1;
        while(*read==' ') ++read;
        *write++=0;
    }
    return int(count);
}
inline bool deleteVerb(const char* verb) {return !strcmp(verb,"rm") || !strcmp(verb,"rmdir");}
struct ShellWalk { unsigned entries=0; uint32_t started=0; };
template<typename Filesystem,typename Clock>
bool walkSd(Filesystem& fs,const char* path,bool deleting,bool recursive,TerminalResult& out,Clock& clock,ShellWalk& budget,unsigned depth=0,const char* mode="find",uint64_t* bytes=nullptr) {
    if(depth>16 || ++budget.entries>4096 || uint32_t(clock.now()-budget.started)>10000) {out.fail("Traversal limit; may be partial");return false;}
    auto file=fs.open(path,"r");if(!file){out.fail("Cannot open path");return false;}
    bool directory=file.isDirectory();
    if(!deleting) {if(bytes && !directory)*bytes+=file.size();if(strcmp(mode,"du")) {char row[39];if(!strcmp(mode,"tree") && depth){const char* base=strrchr(path,'/');snprintf(row,sizeof(row),"%*s|-- %s%s",int(depth-1)*2,"",base?base+1:path,directory?"/":"");}else snprintf(row,sizeof(row),"%s%s",path,directory?"/":"");out.line(row);}}
    if(directory) {
        while(true) {
            auto child=file.openNextFile();if(!child)break;
            char childPath[128];const char* name=child.name();const char* base=strrchr(name,'/');if(base)name=base+1;
            bool valid=sdPath(path,name,childPath,sizeof(childPath));child.close();
            if(!valid){file.close();out.fail("Path too long");return false;}
            if(deleting && !recursive){file.close();out.fail("Directory is not empty; use rm -r");return false;}
            if(!walkSd(fs,childPath,deleting,recursive,out,clock,budget,depth+1,mode,bytes)){file.close();return false;}
            clock.yield();
            // Reopen after deletion: FAT directory iterators can skip removed entries.
            if(deleting){file.close();file=fs.open(path,"r");if(!file){out.fail("Cannot reopen directory");return false;}}
        }
    }
    file.close();
    if(deleting) {if(!directory && !noteRecoveryCleanup(fs,path,true)){out.fail("Note recovery cleanup failed");return false;}bool ok=directory?fs.rmdir(path):fs.remove(path);if(!ok){out.fail("Delete failed; may be partial");return false;}out.mutated=true;}
    return true;
}
template<typename Filesystem,typename Clock>
void executeSd(Filesystem& fs,const char* command,const char* cwd,bool confirmed,TerminalResult& out,Clock& clock) {
    snprintf(out.cwd,sizeof(out.cwd),"%s",cwd);
    char buffer[TerminalCommandLimit+1]; snprintf(buffer,sizeof(buffer),"%s",command); char* args[24]{};
    int count=shellTokens(buffer,args,24); if(count==0) return;
    if(count<0) { out.fail("Malformed command / too many arguments"); return; }
    const char* verb=args[0];
    if(!strcmp(verb,"help") && count==1) { out.line("SD:/ writable; SYS:/ read-only"); out.line("ls [-l] [-a], cd, pwd, clear"); out.line("mkdir [-p], touch, cp, mv"); out.line("rm [-r], rmdir: confirm [y/N]"); out.line("cat head tail wc grep find tree du df"); out.line("echo text [> file | >> file]"); out.line("cp/mv refuse existing destinations"); out.line("Quotes allow spaces in paths"); out.line("Fn up/down: history; left/right: scroll"); return; }
    if(!strcmp(verb,"pwd") && count==1) { out.line(out.cwd); return; }
    if(!strcmp(verb,"clear") && count==1)return;
    if(!strcmp(verb,"df") && count==1){char row[39];snprintf(row,sizeof(row),"SD total %llu B",(unsigned long long)fs.totalBytes());out.line(row);snprintf(row,sizeof(row),"Used %llu B",(unsigned long long)fs.usedBytes());out.line(row);return;}
    if(!strcmp(verb,"echo")) {
        int redirect=0;for(int i=1;i<count;++i)if(!strcmp(args[i],">") || !strcmp(args[i],">>")){redirect=i;break;}
        char text[TerminalCommandLimit+2]{};size_t n=0;
        for(int i=1;i<(redirect?redirect:count);++i){if(i>1)text[n++]=' ';size_t len=strlen(args[i]);memcpy(text+n,args[i],len);n+=len;}text[n++]='\n';text[n]=0;
        if(!redirect){out.line(text);return;}
        char path[128];if(redirect+2!=count || !sdPath(cwd,args[redirect+1],path,sizeof(path))){out.fail("Use echo text > SD-file");return;}
        auto existing=fs.open(path,"r");bool directory=existing && existing.isDirectory();existing.close();if(directory){out.fail("Not a regular file");return;}
        auto target=fs.open(path,!strcmp(args[redirect],">>")?"a":"w");if(!target){out.fail("Cannot write file");return;}out.mutated=true;bool ok=target.write((const uint8_t*)text,n)==n;target.flush();target.close();if(!ok)out.fail("Write failed; may be partial");return;
    }
    bool recursive=count==3 && !strcmp(args[1],"-r");
    if(deleteVerb(verb)) {
        if(count!=2 && !(recursive && !strcmp(verb,"rm"))){out.fail("Use rm [-r] path or rmdir path");return;}
        char path[128];if(!sdPath(cwd,args[recursive?2:1],path,sizeof(path)) || !strcmp(path,"/")){out.fail("Cannot delete root / invalid SD path");return;}
        if(!confirmed){out.fail("Deletion requires confirmation [y/N]");return;}
        auto file=fs.open(path,"r");if(!file){out.fail("Path not found");return;}bool directory=file.isDirectory();file.close();
        if(!strcmp(verb,"rmdir") && !directory){out.fail("Not a directory");return;}
        ShellWalk budget;budget.started=clock.now();if(walkSd(fs,path,true,recursive,out,clock,budget)){out.line("Deleted");size_t length=strlen(path);if(!strncmp(cwd,path,length) && (cwd[length]==0 || cwd[length]=='/'))snprintf(out.cwd,sizeof(out.cwd),"/");}return;
    }
    if(!strcmp(verb,"find") || !strcmp(verb,"tree") || !strcmp(verb,"du")) {
        char path[128];if(count>2 || !sdPath(cwd,count==2?args[1]:".",path,sizeof(path))){out.fail("Use command [SD-path]");return;}
        ShellWalk budget;budget.started=clock.now();uint64_t bytes=0;walkSd(fs,path,false,true,out,clock,budget,0,verb,&bytes);
        if(!strcmp(verb,"du")){char row[39];snprintf(row,sizeof(row),"%llu B",(unsigned long long)bytes);out.line(row);}else if(out.count==TerminalLines)out.lines[TerminalLines-1][0]=0,out.count--,out.line("... output limited");return;
    }
    if(!strcmp(verb,"head") || !strcmp(verb,"tail") || !strcmp(verb,"wc") || !strcmp(verb,"grep")) {
        bool grep=!strcmp(verb,"grep");if(count!=(grep?3:2)){out.fail("Use command file / grep text file");return;}
        char path[128];if(!sdPath(cwd,args[grep?2:1],path,sizeof(path))){out.fail("Invalid SD path");return;}
        auto file=fs.open(path,"r");if(!file || file.isDirectory()){file.close();out.fail("Not a regular file");return;}
        char line[161]{},last[10][39]{};unsigned n=0,lines=0,rows=0,words=0,bytes=0;bool word=false;uint32_t start=clock.now();
        unsigned match=0,patternLength=grep?strlen(args[1]):0;uint8_t prefix[161]{};bool matched=grep && !patternLength;
        if(grep)for(unsigned i=1,j=0;i<patternLength;++i){while(j && args[1][i]!=args[1][j])j=prefix[j-1];if(args[1][i]==args[1][j])++j;prefix[i]=j;}
        while(file.available()) {
            if(uint32_t(clock.now()-start)>10000){out.fail("Read timeout");break;}
            int c=file.read();if(c<0){out.fail("Read failed");break;}++bytes;
            if(grep && patternLength && c!='\n'){while(match && c!=args[1][match])match=prefix[match-1];if(c==args[1][match])++match;if(match==patternLength){matched=true;match=prefix[match-1];}}
            if(isspace((unsigned char)c))word=false;else if(!word){++words;word=true;}
            if(c!='\n' && n<160)line[n++]=c>=32&&c<=126?char(c):'?';
            if(c=='\n' || !file.available()){++rows;line[n]=0;if(c=='\n')++lines;
                if(!strcmp(verb,"tail")){memmove(last,last+1,sizeof(last)-sizeof(last[0]));snprintf(last[9],39,"%.38s",line);}
                else if((!strcmp(verb,"head") && lines<=10) || (grep && matched))out.line(line);
                n=0;match=0;matched=grep && !patternLength;
                if(!strcmp(verb,"head") && lines==10)break;
            }if(bytes%256==0)clock.yield();
        }file.close();
        if(!strcmp(verb,"tail"))for(unsigned i=rows<10?10-rows:0;i<10;++i)out.line(last[i]);
        if(!strcmp(verb,"wc")){char row[39];snprintf(row,39,"%u %u %u (lines words bytes)",lines,words,bytes);out.line(row);}return;
    }
    bool longList=false,all=false,parents=false;
    if(!strcmp(verb,"ls")){int i=1;while(i<count && args[i][0]=='-'){if(!strcmp(args[i],"-l"))longList=true;else if(!strcmp(args[i],"-a"))all=true;else if(!strcmp(args[i],"-la") || !strcmp(args[i],"-al"))longList=all=true;else{out.fail("Use ls [-l] [-a] [path]");return;}++i;}for(int j=1;i<count;++i,++j)args[j]=args[i];count=count-i+1;}
    if(!strcmp(verb,"mkdir") && count==3 && !strcmp(args[1],"-p")){parents=true;args[1]=args[2];count=2;}
    bool ls=!strcmp(verb,"ls"),cd=!strcmp(verb,"cd"),cat=!strcmp(verb,"cat"),touch=!strcmp(verb,"touch"),mkdir=!strcmp(verb,"mkdir"),cp=!strcmp(verb,"cp"),mv=!strcmp(verb,"mv");
    if(!(ls||cd||cat||touch||mkdir||cp||mv)) { out.fail("Unknown command; type help"); return; }
    if(count!=(cp||mv?3:2) && !(ls && count==1) && !(cd && count==1)) { out.fail("Wrong argument count; type help"); return; }
    if(count>1 && args[1][0]=='-') { out.fail("Command flags are not supported"); return; }
    char path[128],destination[128];
    if(!sdPath(cwd,count>1?args[1]:cd?"/":".",path,sizeof(path)) || ((cp||mv) && !sdPath(cwd,args[2],destination,sizeof(destination)))) { out.fail("Access outside SD is not allowed"); return; }
    if(cd || ls || cat ||  cp || mv) {
        auto file=fs.open(path,"r");
        if(!file) { out.fail("File or directory not found"); return; }
        bool directory=file.isDirectory();
        if(cd) { file.close(); if(!directory) out.fail("Not a directory"); else snprintf(out.cwd,sizeof(out.cwd),"%s",path); return; }
        if(ls) {
            if(!directory) { file.close(); out.fail("Not a directory"); return; }
            unsigned scanned=0; uint32_t start=clock.now();
            while(scanned++<128 && out.count<TerminalLines-1) {
                if(uint32_t(clock.now()-start)>10000) { out.fail("Directory timeout"); break; }
                auto entry=file.openNextFile(); if(!entry) break;
                const char* name=entry.name(); const char* base=strrchr(name,'/'); if(base) name=base+1;
                if(!all && name[0]=='.'){entry.close();continue;}
                char row[TerminalColumns+1]; if(longList)snprintf(row,sizeof(row),"%c %llu %s",entry.isDirectory()?'d':'-',(unsigned long long)entry.size(),name);else snprintf(row,sizeof(row),"%s%s",name,entry.isDirectory()?"/":""); out.line(row); entry.close(); clock.yield();
            }
            file.close(); if(!out.count) out.line("(empty)"); else if(out.count>=TerminalLines-1 || scanned>128) out.line("... listing limited"); return;
        }
        if(directory && !mv) { file.close(); out.fail("Command requires a regular file"); return; }
        if(cat) {
            char line[TerminalColumns+1]{}; unsigned n=0,bytes=0; uint32_t start=clock.now();
            while(file.available() && bytes++<4096 && out.count<TerminalLines-1) {
                if(uint32_t(clock.now()-start)>10000) { out.fail("Read timeout"); break; }
                int c=file.read(); if(c<0) { out.fail("Read failed"); break; }
                if(c=='\r') continue;
                if(c=='\n') { line[n]=0; out.line(line); n=0; }
                else { line[n++]=c>=32 && c<=126?char(c):'?'; if(n==TerminalColumns) { line[n]=0; out.line(line); n=0; } }
                if(bytes%128==0) clock.yield();
            }
            if(n) {line[n]=0;out.line(line);} if(file.available()) out.line("... text limited to 4 KB / 64 lines"); file.close(); return;
        }
        if(fs.exists(destination)) {file.close();out.fail("Destination already exists");return;}
        if(mv) {file.close();if(!strcmp(path,"/") || (directory && !strncmp(destination,path,strlen(path)) && destination[strlen(path)]=='/')){out.fail("Invalid directory destination");return;}if(fs.rename(path,destination)){out.mutated=true;out.line("Moved");}else out.fail("Move failed");return;}
        auto target=fs.open(destination,"w"); if(!target) {file.close();out.fail("Cannot create destination");return;}
        uint8_t chunk[256]; size_t remaining=file.size(); uint32_t start=clock.now(); bool ok=true;
        while(remaining) {
            if(uint32_t(clock.now()-start)>10000) {ok=false;break;}
            size_t n=remaining<sizeof(chunk)?remaining:sizeof(chunk);
            if(file.read(chunk,n)!=int(n) || target.write(chunk,n)!=n) {ok=false;break;}
            remaining-=n; clock.yield();
        }
        target.flush(); target.close();file.close();out.mutated=true;
        if(ok) {out.mutated=true;out.line("Copied");} else {out.fail("Copy failed; partial file retained");} return;
    }
    if(!strcmp(path,"/")) {out.fail("Cannot modify SD root");return;}
    if(mkdir && parents){for(char* c=path+1;;++c){if(*c && *c!='/')continue;char saved=*c;*c=0;auto dir=fs.open(path,"r");bool exists=bool(dir),valid=exists && dir.isDirectory();dir.close();if(exists && !valid){out.fail("Parent is not a directory");return;}if(!exists){if(!fs.mkdir(path)){out.fail("mkdir failed; may be partial");return;}out.mutated=true;}*c=saved;if(!saved)break;}out.line("Directories ready");return;}
    if(mkdir) {if(fs.exists(path)) out.fail("Path already exists"); else if(fs.mkdir(path)){out.mutated=true;out.line("Directory created");}else out.fail("Directory creation failed");return;}
    if(fs.exists(path)) {auto file=fs.open(path,"r");bool valid=file && !file.isDirectory();file.close();if(valid)out.line("File already exists");else out.fail("Not a regular file");return;}
    auto file=fs.open(path,"w"); if(!file) {out.fail("File creation failed");return;} file.close();out.mutated=true;out.line("File created");
}
}
