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
  Interface::get().getLogger() << F("☇ Programmatically configuring Homie") << endl;
  if (_flaggedForReboot) {
    Interface::get().getLogger() << F("✖ Device already configured") << endl;
    return 429; // 429 Too many requests
  }

  Interface::get().getLogger() << F("Generating config json") << endl;

  String thejson;
  thejson += "{	\"name\": \"";
  thejson += DeviceId::get();
  thejson += "\",	\"wifi\": {		\"ssid\": \"";
  thejson += ssid;
  thejson += "\",		\"password\": \"";
  thejson += "12345678";
  thejson += "\"	},	\"mqtt\": {		\"host\": \"demo.example.com\",		\"port\": 1883,		\"base_topic\": \"nodes/\",		\"auth\": false	},	\"ota\": {		\"enabled\": false	},	\"settings\": {		\"aborted\": false,		\"settings_id\": \"\",		\"light_off_at\": 22,		\"light_on_at\": 5	}}";

  StaticJsonBuffer<MAX_JSON_CONFIG_ARDUINOJSON_BUFFER_SIZE> parseJsonBuffer;
  std::unique_ptr<char[]> bodyString = Helpers::cloneString(thejson);
  JsonObject& parsedJson = parseJsonBuffer.parseObject(bodyString.get());  // workaround, cannot pass raw String otherwise JSON parsing fails randomly
  if (!parsedJson.success()) {
    Interface::get().getLogger() << F("✖ Invalid or too big JSON") << endl;
    return 413; // 413 Request entity too large
  }

  ConfigValidationResult configValidationResult = Validation::validateConfig(parsedJson);
  if (!configValidationResult.valid) {
    Interface::get().getLogger() << F("✖ Config file is not valid, reason: ") << configValidationResult.reason << endl;
    return 400; // 400 Bad request
  }

  Interface::get().getConfig().write(parsedJson);

  Interface::get().getLogger() << F("✔ Configured") << endl;

  _flaggedForReboot = true;  // We don't reboot immediately, otherwise the response above is not sent
  _flaggedForRebootAt = millis();

  return 201; // 201 created
}

void BootConfig::loop() {
  Boot::loop();


  if(WiFi.smartConfigDone() && !_flaggedForReboot) {
    Interface::get().getLogger() << F("✔ ESP Touch received configuration") << endl;
    String ssid = WiFi.SSID();
    String psk = WiFi.psk();

    setConfig(ssid, psk);

    WiFi.printDiag(Serial);
  }

  if (_flaggedForReboot){
    if (WiFi.status() == WL_CONNECTED) {
      // WiFi.stopSmartConfig();
      Interface::get().getLogger() << F("✔ Verified WiFi configuration") << WiFi.localIP() << endl;
      if (millis() - _flaggedForRebootAt >= 5000UL) {
        /* code */
        Interface::get().getLogger() << F("↻ Rebooting into normal mode...") << endl;
        Serial.flush();
        ESP.restart();
      }
    } else if (millis() - _flaggedForRebootAt >= 58000UL) {    // Default ESP Touch for android app times out in 58000 ms
      Interface::get().getLogger() << F("✖ Failed ESP Touch failed! (wrong SSID/PW or not compatible with ESP Touch)") << endl;
      _flaggedForReboot = false;
      WiFi.stopSmartConfig();
      WiFi.beginSmartConfig();
    }

  }

}
