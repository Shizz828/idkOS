#include <cassert>
#include <map>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include "../src/terminal/SdShell.h"
struct Fs {
    struct Entry {bool directory=false;std::string text;};std::map<std::string,Entry> data;
    bool failWrite=false,failMkdir=false,failRemove=false,failRead=false;
    struct File {
        Fs* fs=nullptr;std::string path;size_t position=0,next=0;std::vector<std::string> children;
        explicit operator bool()const{return fs && fs->data.count(path);}
        bool isDirectory()const{return fs->data[path].directory;}
        size_t size()const{return fs->data[path].text.size();}
        bool available()const{return position<size();}
        int read(){if(fs->failRead)return -1;return available()?uint8_t(fs->data[path].text[position++]):-1;}
        int read(uint8_t* out,size_t count){if(fs->failRead)return -1;count=std::min(count,size()-position);memcpy(out,fs->data[path].text.data()+position,count);position+=count;return int(count);}
        size_t write(const uint8_t* input,size_t count){if(fs->failWrite)return 0;fs->data[path].text.append(reinterpret_cast<const char*>(input),count);return count;}
        const char* name()const{return path.c_str();}
        File openNextFile(){if(next==children.size())return File{};return fs->open(children[next++].c_str(),"r");}
        void close(){}void flush(){}
    };
    Fs(){data["/"].directory=true;}
    bool exists(const char* path){return data.count(path);}
    bool mkdir(const char* path){if(failMkdir)return false;data[path].directory=true;return true;}
    bool remove(const char* path){return !failRemove && data.erase(path);}
    uint64_t totalBytes(){return 1000000;}uint64_t usedBytes(){return 100;}
    bool rmdir(const char* path){if(!data.count(path)||!data[path].directory)return false;std::string prefix=std::string(path)+"/";for(auto& item:data)if(item.first.find(prefix)==0)return false;return remove(path);}
    bool rename(const char* from,const char* to){if(!exists(from)||exists(to))return false;data[to]=data[from];data.erase(from);return true;}
    File open(const char* path,const char* mode){
        std::string name=path;
        if(*mode=='a' && !data.count(name))return open(path,"w");
        if(*mode=='w'){std::string parent=name.substr(0,name.find_last_of('/'));if(parent.empty())parent="/";if(!data.count(parent)||!data[parent].directory)return File{};data[name]=Entry{};}
        if(!data.count(name))return File{};
        File file;file.fs=this;file.path=name;
        if(data[name].directory)for(auto& item:data){std::string parent=item.first.substr(0,item.first.find_last_of('/'));if(parent.empty())parent="/";if(item.first!=name&&parent==name)file.children.push_back(item.first);}
        return file;
    }

};
struct Clock{uint32_t value=0;uint32_t now(){return value;}void yield(){++value;}};
int main(){
    Fs fs;Clock clock;char cwd[128]="/";char path[128];
    assert(!spark::sdPath("/","../",path,sizeof(path)));assert(!spark::sdPath("/a","../../b",path,sizeof(path)));
    assert(spark::sdPath("/a/b","../c",path,sizeof(path))&&!strcmp(path,"/a/c"));
    assert(!spark::sdPath("/","/a/../../../nvs",path,sizeof(path)));assert(!spark::sdPath("/","bad\\path",path,sizeof(path)));
    assert(spark::managedNote("/IDKOS/NOTES/0123456789ABCDEF.TXT"));
    auto run=[&](const char* cmd,bool approved=false){spark::TerminalResult result;spark::executeSd(fs,cmd,cwd,approved,result,clock);if(result.ok)strcpy(cwd,result.cwd);return result;};
    assert(run("pwd").ok);assert(!strcmp(run("pwd").lines[0],"/"));assert(run("mkdir testdir").ok);assert(run("cd testdir").ok);assert(!strcmp(cwd,"/testdir"));
    assert(run("touch test.txt").ok&&fs.exists("/testdir/test.txt"));fs.data["/testdir/test.txt"].text="hello\nworld";
    assert(run("ls").count==1);assert(!strcmp(run("cat test.txt").lines[0],"hello"));
    assert(!run("cd test.txt").ok);assert(!run("rm test.txt").ok&&fs.exists("/testdir/test.txt"));
    assert(run("cp test.txt copy.txt").ok);assert(!run("cp test.txt copy.txt").ok);assert(run("mv copy.txt moved.txt").ok);
    assert(run("rm test.txt",true).ok&&!fs.exists("/testdir/test.txt"));assert(!run("rm test.txt",true).ok);
    assert(run("cd ..").ok&&!strcmp(cwd,"/"));assert(!run("cd ..").ok&&!strcmp(cwd,"/"));
    assert(!run("cd absent").ok);assert(!run("cat absent").ok);assert(!run("unknown").ok);assert(run("").ok);assert(!run("rm -rf /").ok);
    assert(!run("touch \"unterminated").ok);assert(run("touch \"space name.txt\"").ok);
    fs.failMkdir=true;assert(!run("mkdir broken").ok);fs.failMkdir=false;
    fs.failWrite=true;assert(!run("cp /testdir/moved.txt failed.txt").ok&&fs.exists("/failed.txt"));fs.failWrite=false;
    fs.failRead=true;assert(!run("cat /testdir/moved.txt").ok);fs.failRead=false;
    fs.mkdir("/idkos");fs.mkdir("/idkos/notes");
    const char* managed="/idkos/notes/0123456789abcdef.txt";
    fs.data[managed].text="capture note";fs.data["/idkos/notes/0123456789abcdef.bak"].text="old note";
    fs.data["/idkos/notes/0123456789abcdef.rem"].text="1234567890";
    assert(run("rm /idkos/notes/0123456789abcdef.txt",true).ok);
    assert(!fs.exists(managed)&&!fs.exists("/idkos/notes/0123456789abcdef.bak")&&!fs.exists("/idkos/notes/0123456789abcdef.rem"));
    fs.data["/huge"].text=std::string(300000,'x');assert(run("cp huge copy").ok);
    assert(run("mkdir -p /deep/one/two").ok);assert(fs.exists("/deep/one/two"));
    assert(run("touch /deep/one/two/file").ok);
    assert(!run("rm -r /deep").ok && fs.exists("/deep/one/two/file"));
    assert(!run("rmdir /deep",true).ok);assert(run("rm -r /deep",true).ok && !fs.exists("/deep"));
    assert(run("mkdir empty").ok);assert(run("rm empty",true).ok);assert(!run("rm -r /",true).ok);
    assert(run("echo hello world > output").ok);assert(fs.data["/output"].text=="hello world\n");
    assert(run("echo more >> output").ok);assert(fs.data["/output"].text=="hello world\nmore\n");
    assert(run("head output").ok);assert(run("tail output").ok);assert(run("wc output").ok);assert(run("grep world output").count==1);
    fs.data["/longline"].text=std::string(200,'x')+"needle\n";assert(run("grep needle longline").count==1);
    fs.data["/tailfile"].text="one\ntwo";auto tail=run("tail tailfile");assert(tail.count==2&&!strcmp(tail.lines[1],"two"));
    assert(run("ls -la /").ok);assert(run("find /").ok);assert(run("tree /").ok);assert(run("du /").ok);assert(run("df").ok);
    assert(!run("echo bad > SYS:/info").ok);assert(!run("touch SYS:/info").ok);
}
