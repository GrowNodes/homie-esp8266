#pragma once

#include "Arduino.h"

#include <functional>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include "Boot.hpp"
#include "../Constants.hpp"
#include "../Limits.hpp"
#include "../Datatypes/Interface.hpp"
#include "../Timer.hpp"
#include "../Utils/DeviceId.hpp"
#include "../Utils/Validation.hpp"
#include "../Utils/Helpers.hpp"
#include "../Logger.hpp"
#include "../Strings.hpp"
#include "../../HomieSetting.hpp"
#include "../../StreamingOperator.hpp"

namespace HomieInternals {
class BootConfig : public Boot {
 public:
  BootConfig();
  ~BootConfig();
  void setup();
  void loop();
  int setConfig(const String& ssid, const String& psk);

 private:
  bool _flaggedForReboot;
  uint32_t _flaggedForRebootAt;
};
}  // namespace HomieInternals
