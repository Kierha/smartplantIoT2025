#include "wifi_manager.h"
#include <WiFiManager.h>
#include <Preferences.h>

Preferences preferences;

void setupWiFi()
{
    WiFiManager wm;
    preferences.begin("wifi_config", false);
    bool alreadyConfigured = preferences.getBool("configured", false);

    Serial.println("\n📡 [WiFi] Connexion en cours...");
    if (!wm.autoConnect("ESP32_Config"))
    {
        Serial.println("❌ [WiFi] Échec, redémarrage...");
        delay(3000);
        ESP.restart();
    }

    Serial.println("✅ [WiFi] Connecté !");

    if (!alreadyConfigured)
    {
        Serial.println("🔄 [WiFi] Première connexion, enregistrement...");
        preferences.putBool("configured", true);
        delay(3000);
        ESP.restart();
    }

    preferences.end();
}
