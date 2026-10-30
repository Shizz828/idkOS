#pragma once
#include "SdShell.h"
#include "../core/BuildInfo.h"
#include <esp_partition.h>
#include <esp_system.h>
namespace spark {
// A virtual, read-only view. No flash/NVS write APIs are exposed here.
inline void executeSystem(const char* command,const char* cwd,TerminalResult& out) {
    snprintf(out.cwd,sizeof(out.cwd),"%s",cwd);
    char buffer[TerminalCommandLimit+1];snprintf(buffer,sizeof(buffer),"%s",command);char* args[24]{};int count=shellTokens(buffer,args,24);
    if(!count)return;
    if(count<0){out.fail("Malformed command");return;}
    const char* verb=args[0];
    if(!strcmp(verb,"help")){out.line("SYS:/ READ ONLY virtual filesystem");out.line("ls, cd, pwd, cat, head, help, clear");out.line("/info: runtime / build information");out.line("/partitions: internal flash content");out.line("cat partition: first 256 B as hex");return;}
    if(!strcmp(verb,"clear"))return;
    if(!strcmp(verb,"pwd")){out.line(out.cwd);return;}
    bool ls=!strcmp(verb,"ls"),cd=!strcmp(verb,"cd"),cat=!strcmp(verb,"cat") || !strcmp(verb,"head");
    if(!(ls||cd||cat)){out.fail("SYS:/ is read-only; type help");return;}
    char path[128];if(count>2 || !sdPath(cwd,count==2?args[1]:cd?"/":".",path,sizeof(path))){out.fail("Invalid SYS path");return;}
    bool root=!strcmp(path,"/"),partitions=!strcmp(path,"/partitions");
    if(cd){if(root||partitions)snprintf(out.cwd,sizeof(out.cwd),"%s",path);else out.fail("Not a directory");return;}
    if(ls && root){out.line("info");out.line("partitions/");return;}
    if(ls && partitions){
        auto it=esp_partition_find(ESP_PARTITION_TYPE_ANY,ESP_PARTITION_SUBTYPE_ANY,nullptr);
        while(it){const auto* partition=esp_partition_get(it);char row[39];snprintf(row,sizeof(row),"%s %lu B%s",partition->label,(unsigned long)partition->size,partition->encrypted?" encrypted":"");out.line(row);it=esp_partition_next(it);}return;
    }
    if(cat && !strcmp(path,"/info")){char row[39];snprintf(row,sizeof(row),"%s %s",ProductName,SparkVersion);out.line(row);snprintf(row,sizeof(row),"SDK %s",esp_get_idf_version());out.line(row);snprintf(row,sizeof(row),"Heap %lu B",(unsigned long)esp_get_free_heap_size());out.line(row);snprintf(row,sizeof(row),"Uptime %lu s",(unsigned long)(millis()/1000));out.line(row);out.line("Internal storage: read-only");return;}
    if(cat && !strncmp(path,"/partitions/",12)){
        const auto* partition=esp_partition_find_first(ESP_PARTITION_TYPE_ANY,ESP_PARTITION_SUBTYPE_ANY,path+12);
        if(!partition){out.fail("Partition not found");return;}
        out.line("First 256 bytes (hex; read-only)");uint8_t bytes[8];
        for(size_t offset=0;offset<partition->size && offset<256;offset+=8){size_t n=partition->size-offset<8?partition->size-offset:8;if(esp_partition_read(partition,offset,bytes,n)!=ESP_OK){out.fail("Partition read failed");return;}char row[39];int used=snprintf(row,sizeof(row),"%04x:",(unsigned)offset);for(size_t i=0;i<n;++i)used+=snprintf(row+used,sizeof(row)-used," %02x",bytes[i]);out.line(row);}return;
    }
    out.fail("SYS path not found / wrong type");
}
}
