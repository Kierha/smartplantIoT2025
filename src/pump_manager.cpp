#include <Arduino.h>
#include "pump_manager.h"
#include "sensors.h"

#define soilMoistureThreshold 10
#define waterLevelThreshold 40

const int relayPin = 7; // Définition de la broche du relais

void setupPump()
{
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW);
}

void activatePump(int duration)
{
    Serial.printf("🚰 [Pompe] DÉMARRAGE pour %d secondes...\n", duration);

    // Activer réellement la pompe via le relais
    digitalWrite(relayPin, HIGH);

    for (int i = 1; i <= duration; i++)
    {
        Serial.printf("⏳ Pompe active : %d/%d sec\n", i, duration);
        delay(1000); // Attente de 1 seconde
    }

    // Désactiver la pompe après la durée définie
    digitalWrite(relayPin, LOW);

    Serial.println("✅ [Pompe] FIN D'ACTIVATION.");
}

void checkPump()
{
    float moisture = getSoilMoisture();
    float water = getWaterLevel();

    // Vérification que l'humidité est trop basse et qu'on a assez d'eau
    if (moisture >= soilMoistureThreshold)
    {
        return;
    }

    if (water < 30)
    {
        Serial.println("❌ [Pompe] Niveau d'eau critique (<30%), activation annulée pour éviter un pompage à sec !");
        return;
    }

    // Calcul dynamique de la durée d'activation de la pompe
    int duration = map(moisture, 0, soilMoistureThreshold, 15, 5);
    duration = constrain(duration, 5, 15); // Sécurisation (entre 5 et 15s)

    // Ajustement si le niveau d’eau est bas
    if (water < 60)
    {
        Serial.println("⚠️ [Pompe] Niveau d'eau bas, réduction de la durée d'activation.");
        duration = min(duration, 3);
    }

    Serial.printf("🚰 [Auto] Activation de la pompe pour %d secondes (Humidité : %.2f%%, Niveau d'eau : %.2f%%)\n",
                  duration, moisture, water);

    activatePump(duration);
}
