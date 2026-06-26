#pragma once
#include <WiFi.h>
#include <WiFiUdp.h>
#include <esp_wifi.h>

#define CONDOR_TIMEOUT_MS 3000  // ms of silence before falling back to simulation

// Runtime config — load_config() in sd_config.h fills these from SD before setup_condor() runs.
static char     condor_ssid[64] = "MiG-29";
static char     condor_pass[64] = "Vitezovi127";
static uint16_t condor_port     = 55278;

static WiFiUDP       condor_udp;
static unsigned long condor_last_rx = 0;
static bool          condor_wifi_ok = false;

static float condor_var = 0.0f;  // vario   m/s
static float condor_ias = 0.0f;  // airspeed km/h
static float condor_alt = 0.0f;  // altitude m

// Condor sends one key=value pair per line.
// airspeed is in m/s — multiply by 3.6 for the km/h gauge.
static void condor_parse_line(char *line) {
    char *eq = strchr(line, '=');
    if (!eq) return;
    *eq = '\0';
    const char *key = line;
    const char *val = eq + 1;
    if (!*val) return;

    if      (strcmp(key, "airspeed") == 0) condor_ias = atof(val) * 3.6f;
    else if (strcmp(key, "altitude") == 0) condor_alt = atof(val);
    else if (strcmp(key, "vario")    == 0) condor_var = atof(val);
}

void setup_condor() {
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false);
    delay(300);
    WiFi.begin(condor_ssid, condor_pass);
    Serial.print("WiFi connecting");
    uint32_t t = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t < 20000) {
        delay(250);
        Serial.print('.');
    }
    if (WiFi.status() == WL_CONNECTED) {
        esp_wifi_set_ps(WIFI_PS_NONE);
        condor_wifi_ok = true;
        condor_udp.begin(condor_port);
        Serial.printf("\nConnected  IP: %s  port: %u\n", WiFi.localIP().toString().c_str(), condor_port);
    } else {
        Serial.println("\nWiFi failed — simulation mode.");
    }
}

// Returns true only when a new packet was received THIS call.
bool poll_condor() {
    if (!condor_wifi_ok) return false;

    if (WiFi.status() != WL_CONNECTED) {
        static unsigned long last_reconnect = 0;
        if (millis() - last_reconnect > 10000) {
            last_reconnect = millis();
            WiFi.reconnect();
            Serial.println("[Condor] WiFi lost — reconnecting...");
        }
        return false;
    }

    static char pkt[512];
    bool got = false;
    while (condor_udp.parsePacket() > 0) {
        int len = condor_udp.read(pkt, sizeof(pkt) - 1);
        if (len > 0) {
            pkt[len] = '\0';
            condor_last_rx = millis();
            char *line = strtok(pkt, "\r\n");
            while (line) {
                condor_parse_line(line);
                line = strtok(NULL, "\r\n");
            }
            got = true;
        }
        while (condor_udp.available()) condor_udp.read();
    }
    return got;
}

// Returns true while a recent packet has been seen (suppresses simulation).
bool condor_is_live() {
    return condor_last_rx != 0 && (millis() - condor_last_rx) < CONDOR_TIMEOUT_MS;
}

void condor_print_status() {
    Serial.printf("[Condor] WiFi:%s  live:%s  var=%.1f  ias=%.1f  alt=%.0f\n",
        WiFi.status() == WL_CONNECTED ? "OK" : "DOWN",
        condor_is_live() ? "YES" : "NO ",
        condor_var, condor_ias, condor_alt);
}

// Returns the Condor value for the given mode index.
// Must match MODE order in flight_panel.ino: 0=vario, 1=ias, 2=alt
float condor_value_for_mode(uint8_t mode) {
    if (mode == 0) return condor_var;
    if (mode == 1) return condor_ias;
    if (mode == 2) return condor_alt;
    return 0.0f;
}
