#include "settings.h"

Preferences preferences;
SimpleCLI cli;
Command hwIdCmd;
Command resetCmd;
Command wifiCmd;
Command credentialsCmd;
Command wifiClearCmd;
Command credentialsClearCmd;

void handleSettingsCLI() { shell.executeIfInput(); }

String getWifiSSID() { return preferences.getString(WIFI_SSID_PREF_KEY, ""); }

String getWifiPSK() { return preferences.getString(WIFI_PSK_PREF_KEY, ""); }

String getGoliothPSKID() {
  return preferences.getString(GOLIOTH_PSK_ID_PREF_KEY, "");
}

String getGoliothPSK() {
  return preferences.getString(GOLIOTH_PSK_PREF_KEY, "");
}

void hwIdCmdCallback(cmd* c) {
  uint64_t chipid = ESP.getEfuseMac();
  uint16_t chip = (uint16_t)(chipid >> 32);
  char hwid[24];
  snprintf(hwid, 24, "%04X%08X", chip, (uint32_t)chipid);
  Serial.println("# " + String(hwid));
}

void resetCmdCallback(cmd* c) { ESP.restart(); }

void wifiCmdCallback(cmd* c) {
  Command cmd(c);

  Argument keyArg = cmd.getArgument("key");
  Argument valueArg = cmd.getArgument("value");
  String key = keyArg.getValue();
  String value = valueArg.getValue();

  if (key == "ssid") {
    if (value.length() > 0) {
      preferences.putString(WIFI_SSID_PREF_KEY, value);
      Serial.println("# WiFi SSID set to " + value);
    }
  }

  if (key == "psk") {
    if (value.length() > 0) {
      preferences.putString(WIFI_PSK_PREF_KEY, value);
      Serial.println("# WiFi PSK set to " + value);
    }
  }
}

void wifiClearCmdCallback(cmd* c) {
  preferences.remove(WIFI_SSID_PREF_KEY);
  preferences.remove(WIFI_PSK_PREF_KEY);
}

void credentialsCmdCallback(cmd* c) {
  Command cmd(c);

  Argument keyArg = cmd.getArgument("key");
  Argument valueArg = cmd.getArgument("value");
  String key = keyArg.getValue();
  String value = valueArg.getValue();

  if (key == "pskid") {
    if (value.length() > 0) {
      preferences.putString(GOLIOTH_PSK_ID_PREF_KEY, value);
      Serial.println("# PSK ID set to " + value);
    }
  }

  if (key == "psk") {
    if (value.length() > 0) {
      preferences.putString(GOLIOTH_PSK_PREF_KEY, value);
      Serial.println("# PSK set to " + value);
    }
  }
}

void credentialsClearCmdCallback(cmd* c) {
  preferences.remove(GOLIOTH_PSK_ID_PREF_KEY);
  preferences.remove(GOLIOTH_PSK_PREF_KEY);
}

int onShellCommand(int argc, char** argv) {
  // Parse cmd
  String input = "";
  for (int i = 0; i < argc; i++) {
    input += " " + String(argv[i]);
  }
  cli.parse(input);
  return 0;
}

void errorCallback(cmd_error* e) {
  CommandError cmdError(e);  // Create wrapper object

  Serial.print("ERROR: ");
  Serial.println(cmdError.toString());

  if (cmdError.hasCommand()) {
    Serial.print("Did you mean \"");
    Serial.print(cmdError.getCommand().toString());
    Serial.println("\"?");
  }
}

void setupSettings() {
  preferences.begin("app", false);

  cli.setOnError(errorCallback);  // Set error Callback

  hwIdCmd = cli.addCommand("hwid", hwIdCmdCallback);
  resetCmd = cli.addCommand("reset", resetCmdCallback);
  wifiCmd = cli.addCommand("wifi", wifiCmdCallback);
  wifiCmd.addPositionalArgument("key");
  wifiCmd.addPositionalArgument("value");
  wifiClearCmd = cli.addCommand("wifi-clear", wifiClearCmdCallback);
  credentialsCmd = cli.addCommand("golioth", credentialsCmdCallback);
  credentialsCmd.addPositionalArgument("key");
  credentialsCmd.addPositionalArgument("value");
  credentialsClearCmd =
      cli.addCommand("golioth-clear", credentialsClearCmdCallback);

  shell.attach(Serial);
  shell.addCommand(F("hwid"), onShellCommand);
  shell.addCommand(F("wifi"), onShellCommand);
  shell.addCommand(F("wifi-clear"), onShellCommand);
  shell.addCommand(F("golioth"), onShellCommand);
  shell.addCommand(F("golioth-clear"), onShellCommand);
  shell.addCommand(F("reset"), onShellCommand);

  Serial.println(">");
}