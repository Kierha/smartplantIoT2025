#include <Arduino.h>
#include "wifi_manager.h"
#include "firebase_manager.h"
#include "mqtt_manager.h"
#include "sensors.h"
#include "pump_manager.h"

void setup()
{
  Serial.begin(115200);
  setupWiFi();
  setupFirebase();
  setupMQTT();
  setupSensors();
  setupPump();
}

void loop()
{
  checkMQTT();
  checkSensors();
  checkPump();
  delay(5000);
}
