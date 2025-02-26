#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <WiFiManager.h> // Gestion du WiFi
#include <WiFi.h>
#include <Preferences.h> // Stockage de paramètres persistants
#include <FirebaseESP32.h>
#include <ArduinoJson.h>

Adafruit_AHTX0 aht;
Preferences preferences;

// 📌 Définition des broches
const int waterSensorPin = 4;
const int soilMoisturePin = A0;
const int lightSensorPin = 2;
const int relayPin = 7;

// 📌 Calibration du capteur HW-038
const int minWaterLevel = 500;
const int maxWaterLevel = 1657;

// 📌 Identifiants Firebase
#define FIREBASE_HOST "https://smartplant2025-default-rtdb.europe-west1.firebasedatabase.app/"
#define FIREBASE_AUTH "3eVje8w5Afc7x0UZDjxJm55BjgzvcRkv2oHu6oIA"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool wifiConnected = false;
unsigned long lastSentTime = 0;  // 📌 Temps du dernier envoi
const int SEND_INTERVAL = 60000; // 📌 60 secondes (1 min)

// 📌 Stockage des dernières valeurs pour détecter une variation
float lastTemp = -1;
float lastHumidity = -1;
float lastWater = -1;
float lastMoisture = -1;
float lastLight = -1;

const float VARIATION_THRESHOLD = 5.0; // 📌 Seuil de 5% pour déclencher un envoi immédiat

// 📡 Connexion WiFi avec WiFiManager
void setupWiFi()
{
  WiFiManager wm;
  preferences.begin("wifi_config", false);
  bool alreadyConfigured = preferences.getBool("configured", false);

  Serial.println("📡 Démarrage de WiFiManager...");

  if (!wm.autoConnect("ESP32_Config"))
  {
    Serial.println("❌ Échec de connexion WiFi, redémarrage...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("✅ Connecté au WiFi !");
  wifiConnected = true;

  if (!alreadyConfigured)
  {
    Serial.println("🔄 Première connexion, enregistrement...");
    preferences.putBool("configured", true);
    preferences.end();
    delay(3000);
    ESP.restart();
  }

  preferences.end();
}

// 🔥 Configuration Firebase
void setupFirebase()
{
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("🔥 Connexion à Firebase...");
}

// 🔄 Vérification du WiFi en continu
void checkWiFiConnection()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("⚠️ Connexion WiFi perdue, redémarrage...");
    wifiConnected = false;
    delay(5000);
    ESP.restart();
  }
}

// 📤 Envoi des données à Firebase
void sendToFirebase(float temp, float humidity, float water, float moisture, float light)
{
  FirebaseJson json;
  json.set("temperature", temp);
  json.set("humidity", humidity);
  json.set("waterLevel", water);
  json.set("soilMoisture", moisture);
  json.set("lightLevel", light);
  Serial.print("🌡️ Température: ");
  Serial.print(temp);
  Serial.println(" °C");

  Serial.print("💧 Humidité: ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("🚰 Niveau d'eau: ");
  Serial.print(water);
  Serial.println(" %");

  Serial.print("🌱 Humidité du sol: ");
  Serial.print(moisture);
  Serial.println(" %");

  Serial.print("☀️ Luminosité ambiante: ");
  Serial.print(light);
  Serial.println(" %");

  if (Firebase.updateNode(fbdo, "/capteurs", json))
  {
    Serial.println("✅ Données envoyées à Firebase !");
  }
  else
  {
    Serial.print("❌ Erreur Firebase : ");
    Serial.println(fbdo.errorReason());
  }
}

// 📌 Vérifie si la variation est > 5%
bool hasSignificantChange(float newValue, float lastValue)
{
  if (lastValue == -1)
    return true; // Premier envoi forcé
  float variation = abs(newValue - lastValue) / lastValue * 100.0;
  return variation > VARIATION_THRESHOLD;
}

void setup()
{
  Serial.begin(115200);

  pinMode(waterSensorPin, INPUT);
  pinMode(soilMoisturePin, INPUT);
  pinMode(lightSensorPin, INPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  setupWiFi();
  setupFirebase();

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
  checkWiFiConnection();

  Serial.print("📡 État WiFi : ");
  Serial.println(wifiConnected ? "✅ Connecté" : "❌ Déconnecté");

  // 📌 Vérifie si une commande est envoyée via le Moniteur Série
  if (Serial.available())
  {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command == "restart")
    {
      Serial.println("🔄 Redémarrage de l'ESP32 via commande série...");
      delay(1000);
      ESP.restart();
    }
  }

  // 📌 Lecture des capteurs
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  delay(100);

  int waterLevel = analogRead(waterSensorPin);
  float waterPercentage = map(waterLevel, minWaterLevel, maxWaterLevel, 0, 100);
  waterPercentage = constrain(waterPercentage, 0, 100);

  int soilMoisture = analogRead(soilMoisturePin);
  float moisturePercentage = map(soilMoisture, 4095, 0, 0, 100);

  int lightLevel = analogRead(lightSensorPin);
  float lightPercentage = map(lightLevel, 0, 4095, 0, 100);


  // 📌 Vérifie la nécessité d'envoyer les données
  bool sendNow = millis() - lastSentTime > SEND_INTERVAL ||
                 hasSignificantChange(temp.temperature, lastTemp) ||
                 hasSignificantChange(humidity.relative_humidity, lastHumidity) ||
                 hasSignificantChange(waterPercentage, lastWater) ||
                 hasSignificantChange(moisturePercentage, lastMoisture) ||
                 hasSignificantChange(lightPercentage, lastLight);

  if (sendNow)
  {
    sendToFirebase(temp.temperature, humidity.relative_humidity, waterPercentage, moisturePercentage, lightPercentage);
    lastTemp = temp.temperature;
    lastHumidity = humidity.relative_humidity;
    lastWater = waterPercentage;
    lastMoisture = moisturePercentage;
    lastLight = lightPercentage;
    lastSentTime = millis();
  }

  // 🚰 Activation de la pompe SI :
  if (waterPercentage > 40 && moisturePercentage < 30)
  {
    Serial.println("🚰 Pompe activée !");
    digitalWrite(relayPin, HIGH);
  }
  else
  {
    Serial.println("✅ Pompe désactivée.");
    digitalWrite(relayPin, LOW);
  }

  delay(5000);
}
