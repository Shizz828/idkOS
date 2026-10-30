#include <cassert>
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <set>
#include "services_fakes/Preferences.h"
#include "../src/storage/NoteDirectory.h"
struct Fs {
    std::map<std::string,std::string> files;
    std::string failPath;
    unsigned live=0;bool folder=true;
    struct Handle {
        Fs* fs;std::string path;bool directory=false;unsigned offset=0;
        std::vector<std::string> children;
        Handle(Fs* owner,const std::string& name,bool dir):fs(owner),path(name),directory(dir){++fs->live;}
        ~Handle(){--fs->live;}
    };
    struct File {
        std::shared_ptr<Handle> h;
        explicit operator bool()const{return bool(h);}
        bool isDirectory()const{return h && h->directory;}
        const char* name()const{return h->path.c_str();}
        size_t size()const{return h->fs->files[h->path].size();}
        bool available()const{return h && h->offset<size();}
        int read(){return available()?static_cast<unsigned char>(h->fs->files[h->path][h->offset++]):-1;}
        int read(uint8_t* out,size_t n){size_t count=std::min(n,size()-h->offset);memcpy(out,h->fs->files[h->path].data()+h->offset,count);h->offset+=count;return int(count);}
        void close(){h.reset();}
        File openNextFile(){
            if(h->offset>=h->children.size())return File{};
            auto name=h->children[h->offset++];File file;
            file.h=std::make_shared<Handle>(h->fs,name,false);return file;
        }
    };
    File open(const char* path,const char*){
        if(failPath==path)return File{};
        bool dir=std::string(path)=="/idkos/notes" && folder;
        if(!dir && !files.count(path))return File{};
        File result;result.h=std::make_shared<Handle>(this,path,dir);
        if(dir)for(const auto& file:files)result.h->children.push_back(file.first);
        return result;
    }
    std::set<std::string> dirs;bool failRename=false;
    bool exists(const char* path){return files.count(path)||dirs.count(path);}
    bool mkdir(const char* path){return dirs.insert(path).second;}
    bool rename(const char* from,const char* to){
        if(failRename||!exists(from)||exists(to))return false;
        if(dirs.erase(from)){
            dirs.insert(to);std::string prefix=std::string(from)+"/";std::map<std::string,std::string> moved;
            for(auto it=files.begin();it!=files.end();)if(!it->first.compare(0,prefix.size(),prefix)){moved[std::string(to)+"/"+it->first.substr(prefix.size())]=it->second;it=files.erase(it);}else ++it;
            files.insert(moved.begin(),moved.end());return true;
        }
        files[to]=files[from];files.erase(from);return true;
    }
};
struct Clock {uint32_t ms=0;uint32_t now(){return ms;}void yield(){}};
static void noteFolder(){
    using namespace spark;
    // Fresh card: the public folder is created.
    Fs fresh;assert(prepareNoteFolder(fresh) && fresh.dirs.count("/idkos") && fresh.dirs.count("/idkos/notes"));
    assert(prepareNoteFolder(fresh));
    // Card from an earlier build: notes and reminder files move once, byte for byte.
    Fs old;old.dirs={"/spark","/spark/notes"};
    old.files["/spark/notes/0000000000000001.txt"]="First\nbody";
    old.files["/spark/notes/0000000000000001.rem"]="reminder";
    old.files["/spark/notes/0000000000000002.bak"]="Recovered";
    old.failRename=true;
    assert(!prepareNoteFolder(old) && old.files.size()==3 && old.dirs.count("/spark/notes") && !old.dirs.count("/idkos/notes"));
    old.failRename=false;
    assert(prepareNoteFolder(old) && !old.exists("/spark/notes") && old.files.size()==3);
    assert(old.files["/idkos/notes/0000000000000001.txt"]=="First\nbody" && old.files["/idkos/notes/0000000000000001.rem"]=="reminder" && old.files["/idkos/notes/0000000000000002.bak"]=="Recovered");
    // Once /idkos/notes exists, a later /spark/notes is never touched.
    old.dirs.insert("/spark/notes");old.files["/spark/notes/0000000000000009.txt"]="Later";
    assert(prepareNoteFolder(old) && old.files["/spark/notes/0000000000000009.txt"]=="Later" && !old.exists("/idkos/notes/0000000000000009.txt"));
}
int main(){
    using namespace spark;
    noteFolder();
    Fs fs;Clock clock;NoteIndexScan scan;char message[40];
    fs.files["/idkos/notes/0000000000000001.txt"]="First\nbody";
    fs.files["/idkos/notes/0000000000000002.txt"]="Second";
    fs.files["/idkos/notes/0000000000000003.bak"]="Recovered";
    fs.files["/idkos/notes/readme.txt"]="Ignored";
    assert(scanNoteDirectory(fs,scan,0,clock,message,sizeof(message)));
    assert(scan.count==3 && fs.live==0 && fs.exists("/idkos/notes/0000000000000003.txt"));
    NoteStorageState state;scan.commit(state,false);assert(validNoteIndex(state));
    // Repeated scans reopen from the beginning and close every handle.
    for(int i=0;i<20;++i){assert(scanNoteDirectory(fs,scan,0,clock,message,sizeof(message)));assert(scan.count==3 && fs.live==0);}
    // A title-read failure after a partial enumeration cannot erase the live index.
    fs.failPath="/idkos/notes/0000000000000002.txt";
    assert(!scanNoteDirectory(fs,scan,0,clock,message,sizeof(message)));
    assert(fs.live==0 && state.count==3 && scan.count==2);
    fs.failPath.clear();
    clock.ms=16000;
    assert(!scanNoteDirectory(fs,scan,0,clock,message,sizeof(message)));
    assert(fs.live==0 && state.count==3);clock.ms=0;
    fs.folder=false;assert(!scanNoteDirectory(fs,scan,0,clock,message,sizeof(message)));assert(fs.live==0);
    fs.folder=true;fs.files.clear();
    assert(scanNoteDirectory(fs,scan,0,clock,message,sizeof(message)));assert(scan.count==0 && fs.live==0);
    for(unsigned i=0;i<70;++i){char path[64];snprintf(path,sizeof(path),"/idkos/notes/%016x.txt",i);fs.files[path]="Note";}
    assert(scanNoteDirectory(fs,scan,0,clock,message,sizeof(message)));assert(scan.count==64 && scan.truncated && fs.live==0);
}
