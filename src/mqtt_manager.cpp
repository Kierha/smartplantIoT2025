#include "mqtt_manager.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include "pump_manager.h"

#define MQTT_SERVER "192.168.1.17"
#define MQTT_PORT 1883
#define MQTT_TOPIC "smartplant/pompe"

WiFiClient espClient;
PubSubClient client(espClient);

void callbackMQTT(char *topic, byte *payload, unsigned int length)
{
    String message;
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }

    // 📌 Éviter d'afficher un message pour "0"
    if (message == "0")
    {
        return; // On sort immédiatement, évitant l'affichage et la conversion inutile
    }

    Serial.printf("📩 [MQTT] Message reçu: %s\n", message.c_str());

    int duration = message.toInt(); // Convertir en nombre de secondes

    if (duration <= 0 || duration > 30) // Vérification
    {
        Serial.println("❌ [MQTT] Durée invalide, commande ignorée.");
        return;
    }

    Serial.printf("🚰 [MQTT] Activation de la pompe pour %d secondes\n", duration);
    activatePump(duration);

    // 📌 Remettre le bit MQTT à 0 après activation (sans redéclencher la boucle)
    client.publish(MQTT_TOPIC, "0", true);
}

void setupMQTT()
{
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(callbackMQTT);
    Serial.println("📡 [MQTT] Connexion...");
    while (!client.connected())
    {
        if (client.connect("ESP32_Client"))
        {
            Serial.println("✅ [MQTT] Connecté !");
            client.subscribe(MQTT_TOPIC);
        }
        else
        {
            Serial.printf("❌ [MQTT] Échec, erreur : %d\n", client.state());
            delay(5000);
        }
    }
}

void checkMQTT()
{
    if (!client.connected())
    {
        setupMQTT();
    }
    client.loop();
}
