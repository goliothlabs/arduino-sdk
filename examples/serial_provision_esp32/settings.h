
#ifndef settings_h
#define settings_h

#include <Arduino.h>

#define WIFI_SSID_PREF_KEY "wifi_ssid"
#define WIFI_PSK_PREF_KEY "wifi_psk"

#define GOLIOTH_PSK_ID_PREF_KEY "golioth_psk_id"
#define GOLIOTH_PSK_PREF_KEY "golioth_psk"

#include <Preferences.h>
#include <SimpleCLI.h>
#include <SimpleSerialShell.h>

String getWifiSSID();
String getWifiPSK();
String getGoliothPSKID();
String getGoliothPSK();

void handleSettingsCLI();
void setupSettings();

#endif  // settings_h