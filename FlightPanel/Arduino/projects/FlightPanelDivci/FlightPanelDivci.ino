#define FIRMWARE_VERSION "1.0"
#define GITHUB_OWNER     "MavericKraljevic"
#define GITHUB_REPO      "DivciProjects"

#include "LVGL_Driver.h"
#include "TCA9554PWR.h"
#include "SD_Card.h"
#include "vario.h"
#include "altitude.h"
#include "airspeed.h"
#include "condor.h"
#include "flash_config.h"
#include "stats.h"
#include "ota_update.h"
#include <Preferences.h>

#define MODE_BTN_PIN  0
#define MODE_COUNT    4

typedef void (*CreateFn)();
typedef void (*UpdateFn)(float);
static const CreateFn create_fns[MODE_COUNT] = { create_vario, create_ias, create_alt, create_stats };
static const UpdateFn update_fns[MODE_COUNT] = { update_vario, update_ias, update_alt, update_stats };

static uint8_t cur_mode = 0;

static void load_config() {
    File f = SD_MMC.open("/config.ini");
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

void setup() {
    init_flash_storage();   // register USB MSC before USB enumerates
    load_flash_config();    // read config from flash before USB host can unmount it
    Serial.begin(115200);

    Preferences prefs;
    prefs.begin("panel", true);
    cur_mode = prefs.getUChar("mode", 0);
    prefs.end();
    if (cur_mode >= MODE_COUNT) cur_mode = 0;

    pinMode(MODE_BTN_PIN, INPUT_PULLUP);

    Wire.begin(15, 7);
    delay(120);
    TCA9554PWR_Init(0x00);
    Set_EXIO(EXIO_PIN8, Low);
    Backlight_Init();
    Set_Backlight(100);
    LCD_Init();
    SD_Init();
    load_config();
    Lvgl_Init();
    try_sd_ota();
    setup_condor();
    check_github_ota();
    create_fns[cur_mode]();
}

static void handle_mode_button() {
    static uint8_t  last_state  = HIGH;
    static uint32_t last_change = 0;

    uint8_t  state = digitalRead(MODE_BTN_PIN);
    uint32_t now   = millis();

    if (state == last_state) return;
    if (now - last_change < 50) return;
    last_change = now;
    last_state  = state;

    if (state == LOW) {
        cur_mode = (cur_mode + 1) % MODE_COUNT;
        Preferences prefs;
        prefs.begin("panel", false);
        prefs.putUChar("mode", cur_mode);
        prefs.end();
        lv_obj_clean(lv_scr_act());
        create_fns[cur_mode]();
    }
}

void loop() {
    handle_mode_button();
    poll_condor();
    update_fns[cur_mode](condor_is_live() ? condor_value_for_mode(cur_mode) : 0.0f);

    static unsigned long last_status = 0;
    if (millis() - last_status >= 1000) {
        last_status = millis();
        condor_print_status();
    }

    Lvgl_Loop();
}
