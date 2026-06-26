#pragma once
#include <Update.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include "FS.h"
#include "SD_MMC.h"

// Place FlightPanelDivci.ino.bin in the SD card root to trigger an update on next boot.
// The file is deleted after a successful flash so normal boots are unaffected.
//
// IMPORTANT: Arduino IDE → Tools → Partition Scheme must support OTA
// (e.g. "Minimal OTA"). With 16 MB flash any OTA scheme leaves plenty of room.

static void try_sd_ota() {

    File f = SD_MMC.open("/FlightPanelDivci.ino.bin");
    if (!f || f.isDirectory()) {
        if (f) f.close();
        return;
    }

    size_t fw_size = f.size();
    Serial.printf("[OTA] FlightPanelDivci.ino.bin found — %u bytes\n", fw_size);

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);

    lv_obj_t *lbl = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_36, 0);
    lv_label_set_text(lbl, "Updating firmware...");
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, -50);

    lv_obj_t *bar = lv_bar_create(lv_scr_act());
    lv_obj_set_size(bar, 380, 30);
    lv_obj_align(bar, LV_ALIGN_CENTER, 0, 10);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    Lvgl_Loop();

    if (!Update.begin(fw_size)) {
        Serial.printf("[OTA] begin failed: %s\n", Update.errorString());
        lv_label_set_text(lbl, "Update failed!\nCheck partition scheme.");
        Lvgl_Loop();
        f.close();
        delay(4000);
        lv_obj_clean(lv_scr_act());
        return;
    }

    static uint8_t buf[4096];
    size_t written = 0;
    bool ok = true;

    while (f.available() && ok) {
        int n = f.read(buf, sizeof(buf));
        if (n <= 0) break;
        if (Update.write(buf, n) != (size_t)n) {
            Update.abort();
            ok = false;
            break;
        }
        written += n;
        lv_bar_set_value(bar, (written * 100) / fw_size, LV_ANIM_OFF);
        Lvgl_Loop();
    }
    f.close();

    if (ok && Update.end() && Update.isFinished()) {
        SD_MMC.remove("/FlightPanelDivci.ino.bin");
        lv_label_set_text(lbl, "Done! Rebooting...");
        Lvgl_Loop();
        delay(2000);
        ESP.restart();
    } else {
        Serial.printf("[OTA] failed: %s\n", Update.errorString());
        lv_label_set_text(lbl, "Update failed!");
        Lvgl_Loop();
        delay(4000);
        lv_obj_clean(lv_scr_act());
    }
}

// ── GitHub OTA ────────────────────────────────────────────────────────────────
// On boot (after WiFi connects), fetches FlightPanel/version.txt from the repo.
// If it differs from FIRMWARE_VERSION, downloads FlightPanel/FlightPanelDivci.ino.bin and
// flashes it. The device reboots automatically on success.
//
// To deploy an update:
//   1. Bump FIRMWARE_VERSION in the .ino.
//   2. Compile → Sketch → Export Compiled Binary → get FlightPanelDivci.ino.bin.
//   3. Update FlightPanel/version.txt and replace FlightPanel/FlightPanelDivci.ino.bin in
//      the repo. Push. All devices update on next boot.

#define GITHUB_RAW "https://raw.githubusercontent.com/" GITHUB_OWNER "/" GITHUB_REPO "/main/FlightPanel"

static lv_obj_t *s_gh_bar = NULL;

static void gh_ota_progress(int cur, int total) {
    if (s_gh_bar && total > 0) {
        lv_bar_set_value(s_gh_bar, (cur * 100) / total, LV_ANIM_OFF);
        Lvgl_Loop();
    }
}

void check_github_ota() {
    WiFiClientSecure client;
    client.setInsecure();

    // Fetch version.txt to see if an update is available
    HTTPClient http;
    if (!http.begin(client, GITHUB_RAW "/version.txt")) return;
    http.setTimeout(8000);
    if (http.GET() != 200) { http.end(); return; }
    String remote_ver = http.getString();
    http.end();
    remote_ver.trim();
    if (remote_ver == FIRMWARE_VERSION) return;

    // New version — show update screen
    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);

    lv_obj_t *lbl = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_36, 0);
    char buf[80];
    snprintf(buf, sizeof(buf), "v%s -> v%s\nUpdating...", FIRMWARE_VERSION, remote_ver.c_str());
    lv_label_set_text(lbl, buf);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, -50);

    s_gh_bar = lv_bar_create(lv_scr_act());
    lv_obj_set_size(s_gh_bar, 380, 30);
    lv_obj_align(s_gh_bar, LV_ALIGN_CENTER, 0, 30);
    lv_bar_set_range(s_gh_bar, 0, 100);
    lv_bar_set_value(s_gh_bar, 0, LV_ANIM_OFF);
    Lvgl_Loop();

    WiFiClientSecure dl_client;
    dl_client.setInsecure();
    httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    httpUpdate.onProgress(gh_ota_progress);

    t_httpUpdate_return ret = httpUpdate.update(dl_client, GITHUB_RAW "/FlightPanelDivci.ino.bin");

    if (ret != HTTP_UPDATE_OK) {
        lv_label_set_text(lbl, "Update failed!");
        Lvgl_Loop();
        delay(3000);
        lv_obj_clean(lv_scr_act());
    }
    // HTTP_UPDATE_OK triggers automatic reboot
}
