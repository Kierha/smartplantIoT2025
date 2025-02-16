#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>

Adafruit_AHTX0 aht;

const int waterSensorPin = 4;   // Capteur de niveau d'eau (HW-038)
const int soilMoisturePin = A0; // Capteur d'humidité du sol (DFRobot)
const int lightSensorPin = 2;   // Capteur de luminosité (KY-018)
const int relayPin = 7;         // Broche du relais KY-019 (Pompe)

// 📌 Calibration spécifique au capteur HW-038
const int minWaterLevel = 500;  // Valeur brute mesurée à sec
const int maxWaterLevel = 1657; // Valeur brute mesurée totalement immergé

void setup()
{
  Serial.begin(115200);
  pinMode(waterSensorPin, INPUT);
  pinMode(soilMoisturePin, INPUT);
  pinMode(lightSensorPin, INPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // Assurer que la pompe est éteinte au démarrage

  // Initialisation du capteur AHT10
  if (!aht.begin())
  {
    Serial.println("❌ Impossible de trouver le capteur AHT10 ! Redémarrage...");
    delay(3000);
    ESP.restart();
  }
  Serial.println("✅ Capteur AHT10 initialisé !");
}

void loop()
{
  // 🌡️ Lecture température & humidité (AHT10)
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  delay(100); // Stabilisation de la lecture

  Serial.print("🌡️ Température: ");
  Serial.print(temp.temperature);
  Serial.print(" °C, 💧 Humidité: ");
  Serial.print(humidity.relative_humidity);
  Serial.println(" %");

  // 🚰 Lecture niveau d’eau (HW-038)
  int waterLevel = analogRead(waterSensorPin);
  Serial.print("📏 Valeur brute HW-038 : ");
  Serial.println(waterLevel);

  // Correction de la calibration
  float waterPercentage = map(waterLevel, minWaterLevel, maxWaterLevel, 0, 100);
  waterPercentage = constrain(waterPercentage, 0, 100);

  Serial.print("🚰 Niveau d'eau : ");
  Serial.print(waterPercentage);
  Serial.println(" %");

  // 🌱 Lecture humidité du sol (DFRobot)
  int soilMoisture = analogRead(soilMoisturePin);
  float moisturePercentage = map(soilMoisture, 4095, 0, 0, 100);

  Serial.print("🌱 Humidité du sol : ");
  Serial.print(moisturePercentage);
  Serial.println(" %");

  // ☀️ Lecture luminosité ambiante (KY-018)
  int lightLevel = analogRead(lightSensorPin);
  float lightPercentage = map(lightLevel, 0, 4095, 0, 100);

  Serial.print("☀️ Luminosité ambiante : ");
  Serial.print(lightPercentage);
  Serial.println(" %");

  // ⚠️ Alerte si niveau bas
  if (waterPercentage < 20 || moisturePercentage < 20 || lightPercentage < 30)
  {
    Serial.println("⚠️ Alerte : Niveau d’eau, humidité ou lumière trop bas !");
  }

  // 🚰 Activation de la pompe SI :
  // - Le niveau d'eau est supérieur à 40% (évite de pomper dans un réservoir vide)
  // - L'humidité du sol est inférieure à 30% (indique que la plante a besoin d'eau)
  if (waterPercentage > 40)
  {
    Serial.println("🚰 Pompe activée (sol trop sec et niveau d'eau suffisant) !");
    digitalWrite(relayPin, HIGH); // Active le relais (ferme le circuit)
  }
  else
  {
    Serial.println("✅ Pompe désactivée (sol assez humide ou niveau d'eau insuffisant)");
    digitalWrite(relayPin, LOW); // Désactive le relais (ouvre le circuit)
  }

  delay(2000);
}
