#include "BootConfig.hpp"

using namespace HomieInternals;

BootConfig::BootConfig()
: Boot("config")
{
}

BootConfig::~BootConfig() {
}

void BootConfig::setup() {
  Boot::setup();

  if (Interface::get().led.enabled) {
    digitalWrite(Interface::get().led.pin, Interface::get().led.on);
  }

  Interface::get().getLogger() << F("Device ID is ") << DeviceId::get() << endl;
  WiFi.mode(WIFI_STA);
  WiFi.beginSmartConfig();
  Interface::get().getLogger() << F("☢  ESP Touch / Smart Config is now active.") << endl;

}

int BootConfig::setConfig(const String& ssid, const String& psk) {
  Interface::get().getLogger() << F("Programmatically configuring Homie") << endl;
  if (_flaggedForReboot) {
    Interface::get().getLogger() << F("✖ Device already configured") << endl;
    return 429; // 429 Too many requests
  }

  Interface::get().getLogger() << F("Generating config json") << endl;

  StaticJsonBuffer<MAX_JSON_CONFIG_ARDUINOJSON_BUFFER_SIZE> parseJsonBuffer;
  JsonObject& generatedConfig = parseJsonBuffer.createObject();
  generatedConfig["name"] = DeviceId::get();

  JsonObject& wifiObj = generatedConfig.createNestedObject("wifi");
  wifiObj["ssid"] = ssid;
  wifiObj["password"] = psk;

  JsonObject& mqttObj = generatedConfig.createNestedObject("mqtt");
  mqttObj["host"] = "demo.example.com";
  mqttObj["port"] = 1883;
  mqttObj["base_topic"] = "nodes/";
  mqttObj["auth"] = false;

  JsonObject& otaObj = generatedConfig.createNestedObject("ota");
  otaObj["enabled"] = false;

  JsonObject& settingsObj = generatedConfig.createNestedObject("settings");
  settingsObj["aborted"] = false;
  settingsObj["settings_id"] = "undefined";
  settingsObj["light_off_at"] = 22;
  settingsObj["light_on_at"] = 5;


  ConfigValidationResult configValidationResult = Validation::validateConfig(generatedConfig);
  if (!configValidationResult.valid) {
    Interface::get().getLogger() << F("✖ Config file is not valid, reason: ") << configValidationResult.reason << endl;
    return 400; // 400 Bad request
  }

  Interface::get().getConfig().write(generatedConfig);

  Interface::get().getLogger() << F("✔ Configured") << endl;

  _flaggedForReboot = true;  // We don't reboot immediately, otherwise the response above is not sent
  _flaggedForRebootAt = millis();

  return 201; // 201 created
}

void BootConfig::loop() {
  Boot::loop();


  if(WiFi.smartConfigDone() && !_flaggedForReboot) {
    Interface::get().getLogger() << F("⇄ ESP Touch received configuration") << endl;

    setConfig(WiFi.SSID(), WiFi.psk());
  }

  if (_flaggedForReboot){
    if (WiFi.status() == WL_CONNECTED) {
      if (millis() - _flaggedForRebootAt >= 5000UL) { // wait for ESP Touch to send IP info to app.
        // WiFi.localIP()
        Interface::get().getLogger() << F("✔ Verified WiFi configuration") << endl;
        Interface::get().getLogger() << F("↻ Rebooting into normal mode...") << endl;
        Serial.flush();
        ESP.restart();
      }
    } else if (millis() - _flaggedForRebootAt >= 58000UL) {    // Default ESP Touch for android app times out in 58000 ms
      Interface::get().getLogger() << F("✖ ESP Touch failed (wrong SSID/PW or not compatible with ESP Touch)") << endl;
      _flaggedForReboot = false;
      WiFi.stopSmartConfig();
      WiFi.beginSmartConfig();
    }

  }

}
