#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
constexpr int ESP_PARTITION_TYPE_ANY=255,ESP_PARTITION_SUBTYPE_ANY=255,ESP_OK=0;
struct esp_partition_t {const char* label;size_t size;bool encrypted;};
using esp_partition_iterator_t=const esp_partition_t*;
static const esp_partition_t testPartition={"nvs",4096,false};
inline esp_partition_iterator_t esp_partition_find(int,int,const char*){return &testPartition;}
inline const esp_partition_t* esp_partition_get(esp_partition_iterator_t it){return it;}
inline esp_partition_iterator_t esp_partition_next(esp_partition_iterator_t){return nullptr;}
inline const esp_partition_t* esp_partition_find_first(int,int,const char* label){return !strcmp(label,"nvs")?&testPartition:nullptr;}
inline int esp_partition_read(const esp_partition_t*,size_t,void* buffer,size_t length){memset(buffer,0xab,length);return ESP_OK;}
