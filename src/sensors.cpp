#include "sensors.h"
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_Sensor.h>
#include "firebase_manager.h"

// 📌 Définition des capteurs
Adafruit_AHTX0 aht;

const int waterSensorPin = 4;
const int soilMoisturePin = A0;
const int lightSensorPin = 2;

// 📌 Calibration du capteur HW-038 (Niveau d'eau)
const int minWaterLevel = 500;
const int maxWaterLevel = 1657;

// 📌 Stockage des dernières valeurs pour détecter une variation
float lastTemp = -1, lastHumidity = -1, lastWater = -1, lastMoisture = -1, lastLight = -1;
const float VARIATION_THRESHOLD = 80.0;

// 📌 Timers pour la gestion des envois et affichages
unsigned long lastSentTime = 0;
unsigned long lastDisplayTime = 0;
const int DISPLAY_INTERVAL = 30000; // Affichage toutes les 30s
const int SEND_INTERVAL = 60000;    // Envoi Firebase toutes les 60s

// Vérifie si une variation dépasse le seuil
bool hasSignificantChange(float newValue, float &lastValue, const String &variableName)
{
    if (lastValue == -1) // Première lecture, éviter faux positifs
    {
        lastValue = newValue;
        return false;
    }

    float variation = 0;
    if (lastValue > 0) // Protection contre la division par zéro
    {
        variation = (newValue - lastValue) / lastValue * 100.0; // Calcul de la variation en pourcentage
    }

    if (abs(variation) > VARIATION_THRESHOLD) // Vérifie si la variation dépasse le seuil
    {
        Serial.printf("\n⚠️ [Changement] %s a changé de %.2f%% (%s %.2f → %.2f)\n",
                      variableName.c_str(), abs(variation),
                      (variation > 0) ? "⬆ Augmentation" : "⬇ Diminution",
                      lastValue, newValue);

        lastValue = newValue; // Mise à jour après détection
        return true;
    }

    lastValue = newValue; // Mise à jour en continu
    return false;
}

// Initialisation des capteurs
void setupSensors()
{
    if (!aht.begin())
    {
        Serial.println("❌ [Capteur] AHT10 non détecté ! Redémarrage...");
        delay(3000);
        ESP.restart();
    }
    Serial.println("✅ [Capteur] AHT10 initialisé !");
    pinMode(waterSensorPin, INPUT);
    pinMode(soilMoisturePin, INPUT);
    pinMode(lightSensorPin, INPUT);
}

// Récupération des valeurs des capteurs
void readTemperatureAndHumidity(float &temperature, float &humidity)
{
    sensors_event_t humEvent, tempEvent;
    aht.getEvent(&humEvent, &tempEvent);
    temperature = tempEvent.temperature;
    humidity = humEvent.relative_humidity;
}

float getWaterLevel()
{
    int waterLevel = analogRead(waterSensorPin);
    return constrain(map(waterLevel, minWaterLevel, maxWaterLevel, 0, 100), 0, 100);
}

float getSoilMoisture()
{
    int soilMoisture = analogRead(soilMoisturePin);
    return constrain(map(soilMoisture, 4095, 0, 0, 100), 0, 100);
}

float getLightLevel()
{
    int lightLevel = analogRead(lightSensorPin);
    return constrain(map(lightLevel, 0, 4095, 0, 100), 0, 100);
}

// Vérification des capteurs et envoi des données si nécessaire
void checkSensors()
{
    float temp, humidity;
    readTemperatureAndHumidity(temp, humidity);
    float water = getWaterLevel();
    float moisture = getSoilMoisture();
    float light = getLightLevel();

    bool shouldSend = false;

    shouldSend |= hasSignificantChange(temp, lastTemp, "Température");
    shouldSend |= hasSignificantChange(humidity, lastHumidity, "Humidité");
    shouldSend |= hasSignificantChange(water, lastWater, "Niveau d'eau");
    shouldSend |= hasSignificantChange(moisture, lastMoisture, "Humidité du sol");
    shouldSend |= hasSignificantChange(light, lastLight, "Luminosité");

    // Affichage des valeurs toutes les 30 secondes
    if (millis() - lastDisplayTime > DISPLAY_INTERVAL)
    {
        Serial.println("\n📊 [Affichage] État actuel des capteurs :");
        Serial.printf("🌡️ Température : %.2f°C\n", temp);
        Serial.printf("💧 Humidité : %.2f%%\n", humidity);
        Serial.printf("🚰 Niveau d'eau : %.2f%%\n", water);
        Serial.printf("🌱 Humidité du sol : %.2f%%\n", moisture);
        Serial.printf("☀️ Luminosité : %.2f%%\n", light);
        lastDisplayTime = millis();
    }

    // Envoi des données si un changement significatif est détecté ou après 60s
    if (shouldSend || millis() - lastSentTime > SEND_INTERVAL)
    {
        Serial.println("\n📤 [Firebase] Envoi des nouvelles données !");
        sendToFirebase(temp, humidity, water, moisture, light);
        lastSentTime = millis();
    }
}
