#include "firebase_manager.h"
#include <FirebaseESP32.h>

#define FIREBASE_HOST "https://smartplant2025-default-rtdb.europe-west1.firebasedatabase.app/"
#define FIREBASE_AUTH "3eVje8w5Afc7x0UZDjxJm55BjgzvcRkv2oHu6oIA"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

void setupFirebase()
{
    config.host = FIREBASE_HOST;
    config.signer.tokens.legacy_token = FIREBASE_AUTH;
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    Serial.println("🔥 [Firebase] Connecté !");
}

void sendToFirebase(float temp, float humidity, float water, float moisture, float light)
{
    FirebaseJson json;
    json.set("temperature", temp);
    json.set("humidity", humidity);
    json.set("waterLevel", water);
    json.set("soilMoisture", moisture);
    json.set("lightLevel", light);

    Serial.println("📤 [Firebase] Tentative d'envoi des données...");
    if (Firebase.updateNode(fbdo, "/capteurs", json))
    {
        Serial.println("✅ [Firebase] Données envoyées avec succès !");
    }
    else
    {
        Serial.printf("❌ [Firebase] Erreur lors de l'envoi : %s\n", fbdo.errorReason().c_str());
    }
}
