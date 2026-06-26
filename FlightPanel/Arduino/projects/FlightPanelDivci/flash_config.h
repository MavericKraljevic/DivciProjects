#pragma once
#include <FFat.h>
#include <USB.h>
#include <USBMSC.h>
#include <esp_partition.h>
#include "condor.h"

static USBMSC s_msc;
static const esp_partition_t *s_fat_part = NULL;
static bool s_fat_mounted = false;

static int32_t fc_read(uint32_t lba, uint32_t offset, void *buf, uint32_t size) {
    if (!s_fat_part) return -1;
    return esp_partition_read(s_fat_part, (size_t)lba * 512 + offset, buf, size) == ESP_OK
           ? (int32_t)size : -1;
}

// Flash requires read-modify-write on 4 KB erase pages.
static int32_t fc_write(uint32_t lba, uint32_t offset, uint8_t *buf, uint32_t size) {
    if (!s_fat_part) return -1;
    size_t addr      = (size_t)lba * 512 + offset;
    size_t page_addr = addr & ~(size_t)4095;
    static uint8_t page[4096];
    if (esp_partition_read(s_fat_part, page_addr, page, 4096)   != ESP_OK) return -1;
    memcpy(page + (addr - page_addr), buf, size);
    if (esp_partition_erase_range(s_fat_part, page_addr, 4096)  != ESP_OK) return -1;
    if (esp_partition_write(s_fat_part, page_addr, page, 4096)  != ESP_OK) return -1;
    return (int32_t)size;
}

// Unmount FFat while USB host owns the drive, remount when it releases.
static bool fc_startstop(uint8_t, bool start, bool) {
    if (start && s_fat_mounted)    { FFat.end(); s_fat_mounted = false; }
    if (!start && !s_fat_mounted)  { s_fat_mounted = FFat.begin(false, "/ffat", 2); }
    return true;
}

// Must be called BEFORE Serial.begin() so MSC is registered before USB enumerates.
void init_flash_storage() {
    s_fat_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                          ESP_PARTITION_SUBTYPE_DATA_FAT, NULL);
    if (!s_fat_part) return;

    if (!FFat.begin(false, "/ffat", 2)) {
        FFat.format();
        FFat.begin(false, "/ffat", 2);
    }
    s_fat_mounted = true;

    s_msc.vendorID("Maveric");
    s_msc.productID("FlightPanel");
    s_msc.productRevision("1.0");
    s_msc.onRead(fc_read);
    s_msc.onWrite(fc_write);
    s_msc.onStartStop(fc_startstop);
    s_msc.mediaPresent(true);
    s_msc.begin(s_fat_part->size / 512, 512);
}

// Load config.ini from internal flash. SD card (load_config) overrides this if present.
void load_flash_config() {
    if (!s_fat_mounted) return;
    File f = FFat.open("/config.ini");
    if (!f) return;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (!line.length() || line[0] == '#') continue;
        int open  = line.indexOf('[');
        int close = line.indexOf(']');
        int eq    = line.indexOf('=');
        if (open < 0 || close < 0 || eq < 0 || close > eq) continue;
        String key = line.substring(open + 1, close);
        String val = line.substring(eq + 1);
        key.trim(); val.trim();
        if      (key.equalsIgnoreCase("SSID")) val.toCharArray(condor_ssid, sizeof(condor_ssid));
        else if (key.equalsIgnoreCase("PW"))   val.toCharArray(condor_pass, sizeof(condor_pass));
        else if (key.equalsIgnoreCase("PORT")) condor_port = (uint16_t)val.toInt();
    }
    f.close();
}
