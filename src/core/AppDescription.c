/*
 * SPDX-FileCopyrightText: 2017-2021 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 *
 * ESP-IDF v4.4.7 app_update/esp_app_desc.c, with idkOS release identity.
 * Providing this whole translation unit preserves the SDK's description/SHA
 * accessors without linking Arduino's prebuilt generic app descriptor.
 */
#include <sys/param.h>
#include "esp_ota_ops.h"
#include "esp_attr.h"
#include "esp_idf_version.h"
#include "sdkconfig.h"
_Static_assert(ESP_IDF_VERSION == ESP_IDF_VERSION_VAL(4,4,7), "Review app description for a different SDK");
const __attribute__((section(".rodata_desc"))) esp_app_desc_t esp_app_desc = {
    .magic_word = ESP_APP_DESC_MAGIC_WORD,
#ifdef CONFIG_BOOTLOADER_APP_SECURE_VERSION
    .secure_version = CONFIG_BOOTLOADER_APP_SECURE_VERSION,
#endif
    .version = "1.1.2",
    .project_name = "idkOS ADV",
    .time = __TIME__,
    .date = __DATE__,
    .idf_ver = "v4.4.7",
};
const esp_app_desc_t *esp_ota_get_app_description(void) { return &esp_app_desc; }
static inline char IRAM_ATTR to_hex_digit(unsigned val) { return val<10 ? '0'+val : 'a'+val-10; }
__attribute__((constructor)) void esp_ota_init_app_elf_sha256(void) { esp_ota_get_app_elf_sha256(NULL,0); }
int IRAM_ATTR esp_ota_get_app_elf_sha256(char* dst,size_t size) {
    static char s_app_elf_sha256[CONFIG_APP_RETRIEVE_LEN_ELF_SHA/2];
    static bool first_call=true;
    if(first_call) {
        first_call=false;
        const volatile uint8_t* src=esp_app_desc.app_elf_sha256;
        for(size_t i=0;i<sizeof(s_app_elf_sha256);++i)s_app_elf_sha256[i]=src[i];
    }
    if(dst==NULL || size==0)return 0;
    size_t n=MIN((size-1)/2,sizeof(s_app_elf_sha256));
    for(size_t i=0;i<n;++i) {
        dst[2*i]=to_hex_digit(s_app_elf_sha256[i]>>4);
        dst[2*i+1]=to_hex_digit(s_app_elf_sha256[i]&0xf);
    }
    dst[2*n]=0;
    return 2*n+1;
}
