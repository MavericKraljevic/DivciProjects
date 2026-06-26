#pragma once
#include <lvgl.h>
#include "LVGL_Driver.h"
#include "condor.h"

static lv_obj_t *stats_lbl = NULL;

void create_stats() {
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);

    stats_lbl = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(stats_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(stats_lbl, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_align(stats_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(stats_lbl, 440);
    lv_label_set_long_mode(stats_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_align(stats_lbl, LV_ALIGN_CENTER, 0, 0);

    char buf[400];
    if (condor_wifi_ok) {
        IPAddress ip = WiFi.localIP();
        snprintf(buf, sizeof(buf), "v" FIRMWARE_VERSION "\nWiFi connected\n%s\n%s\n%d.%d.%d.255\n%u",
                 condor_ssid, condor_pass, ip[0], ip[1], ip[2], condor_port);
        lv_label_set_text(stats_lbl, buf);
    } else {
        lv_label_set_text(stats_lbl, "v" FIRMWARE_VERSION "\nNo WiFi\nScanning...");
        Lvgl_Loop();
        WiFi.disconnect(true);
        WiFi.mode(WIFI_STA);
        delay(200);
        int n = WiFi.scanNetworks();
        if (n <= 0) {
            snprintf(buf, sizeof(buf), "v" FIRMWARE_VERSION "\nNo WiFi\n%s\n%s\n\nNo networks found",
                     condor_ssid, condor_pass);
        } else {
            String s = String("v" FIRMWARE_VERSION "\nNo WiFi\n") + condor_ssid + "\n" + condor_pass + "\n\n";
            for (int i = 0; i < min(n, 5); i++) {
                s += WiFi.SSID(i);
                if (i < min(n, 5) - 1) s += "\n";
            }
            s.toCharArray(buf, sizeof(buf));
        }
        lv_label_set_text(stats_lbl, buf);
        WiFi.scanDelete();
    }
}

void update_stats(float) {}
