#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "DHT.h"

// ====== CONFIGURACIÓN WIFI ======
#define WIFI_SSID "Redmi Note 12 Pro"
#define WIFI_PASSWORD "78933491"

// ====== CONFIGURACIÓN FIREBASE ======
#define API_KEY "AIzaSyDsEb_YAGcr9S9fKjzmJKuA7RdNuJtQZJg"
#define DATABASE_URL "https://huerto-unandes-default-rtdb.firebaseio.com/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ====== PINES SENSORES ======
#define DHTPIN_A 4       // DHT11 Lado A
#define DHTPIN_B 5       // DHT22 Lado B
#define DHTTYPE_A DHT11
#define DHTTYPE_B DHT22
#define SOIL_A 34        // humedad suelo A (ADC)
#define SOIL_B 35        // humedad suelo B (ADC)
#define LDR_A 32         // luz A (ADC)

// ====== PINES ACTUADORES ======
#define FAN_A 14
#define HEATER_A 27
#define PUMP_A 26
#define LIGHT_A 25

#define FAN_B 33
#define HEATER_B 19
#define PUMP_B 18

// ====== OBJETOS ======
DHT dhtA(DHTPIN_A, DHTTYPE_A);
DHT dhtB(DHTPIN_B, DHTTYPE_B);

// ====== VARIABLES DE CONTROL ======
struct Targets {
  float tempMin, tempMax;
  float humMin, humMax;
  float soilMin, soilMax;
  int lightMin, lightMax;
};

Targets targetsA, targetsB;

// ====== FUNCIONES AUXILIARES ======
void logActuator(const char* name, int pin, int state) {
  digitalWrite(pin, state);
  Serial.print(name);
  Serial.println(state ? " ON" : " OFF");
}

// ====== FUNCION LECTURA HUMEDAD SUELO CALIBRADA ======
float readSoil(int pin) {
  int sensorValue = analogRead(pin); // 0 - 4095
  float soilPercent = 100.0 - (sensorValue * 100.0 / 4095.0); // 0 = seco, 100 = mojado
  if (soilPercent < 0) soilPercent = 0;
  if (soilPercent > 100) soilPercent = 100;
  return soilPercent;
}

// ====== SETUP ======
void setup() {
  Serial.begin(115200);

  // Sensores
  dhtA.begin();
  dhtB.begin();

  // Actuadores
  pinMode(FAN_A, OUTPUT);
  pinMode(HEATER_A, OUTPUT);
  pinMode(PUMP_A, OUTPUT);
  pinMode(LIGHT_A, OUTPUT);
  pinMode(FAN_B, OUTPUT);
  pinMode(HEATER_B, OUTPUT);
  pinMode(PUMP_B, OUTPUT);

  // Inicializar actuadores en OFF
  logActuator("FAN_A", FAN_A, LOW);
  logActuator("HEATER_A", HEATER_A, LOW);
  logActuator("PUMP_A", PUMP_A, LOW);
  logActuator("LIGHT_A", LIGHT_A, LOW);
  logActuator("FAN_B", FAN_B, LOW);
  logActuator("HEATER_B", HEATER_B, LOW);
  logActuator("PUMP_B", PUMP_B, LOW);

  // Conectar WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado a WiFi");

  // Firebase config
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = "josueleonardomanriquez@gmail.com";
  auth.user.password = "12341234";
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Firebase listo");
}

// ====== LOOP ======
void loop() {
  // ====== Lecturas ======
  float tempA = dhtA.readTemperature();
  float humA = dhtA.readHumidity();
  float soilA = readSoil(SOIL_A);
  int luxA = analogRead(LDR_A);

  float tempB = dhtB.readTemperature();
  float humB = dhtB.readHumidity();
  float soilB = readSoil(SOIL_B);

  // ====== Mostrar en Monitor Serial ======
  Serial.println("===== LADO A =====");
  Serial.print("Temperatura: "); Serial.print(tempA); Serial.println(" °C");
  Serial.print("Humedad: "); Serial.print(humA); Serial.println(" %");
  Serial.print("Humedad suelo: "); Serial.print(soilA); Serial.println(" %");
  Serial.print("Luz: "); Serial.println(luxA);

  Serial.println("===== LADO B =====");
  Serial.print("Temperatura: "); Serial.print(tempB); Serial.println(" °C");
  Serial.print("Humedad: "); Serial.print(humB); Serial.println(" %");
  Serial.print("Humedad suelo: "); Serial.print(soilB); Serial.println(" %");
  Serial.println("===================");

  // Timestamp
  String timeKey = String(millis() / 1000);

  // ====== Enviar a Firebase ======
  if (Firebase.ready()) {
    Firebase.RTDB.setFloat(&fbdo, "/huerto/sideA/readings/" + timeKey + "/temperatura", tempA);
    Firebase.RTDB.setFloat(&fbdo, "/huerto/sideA/readings/" + timeKey + "/humedad", humA);
    Firebase.RTDB.setFloat(&fbdo, "/huerto/sideA/readings/" + timeKey + "/humedad_suelo", soilA);
    Firebase.RTDB.setInt(&fbdo, "/huerto/sideA/readings/" + timeKey + "/luz", luxA);

    Firebase.RTDB.setFloat(&fbdo, "/huerto/sideB/readings/" + timeKey + "/temperatura", tempB);
    Firebase.RTDB.setFloat(&fbdo, "/huerto/sideB/readings/" + timeKey + "/humedad", humB);
    Firebase.RTDB.setFloat(&fbdo, "/huerto/sideB/readings/" + timeKey + "/humedad_suelo", soilB);
  }

  // ====== Leer targets desde Firebase ======
  if (Firebase.RTDB.getJSON(&fbdo, "/huerto/sideA/targets")) {
    FirebaseJson &json = fbdo.jsonObject();
    FirebaseJsonData result;
    if (json.get(result, "tempMin")) targetsA.tempMin = result.to<float>();
    if (json.get(result, "tempMax")) targetsA.tempMax = result.to<float>();
    if (json.get(result, "humMin"))  targetsA.humMin  = result.to<float>();
    if (json.get(result, "humMax"))  targetsA.humMax  = result.to<float>();
    if (json.get(result, "soilMin")) targetsA.soilMin = result.to<float>();
    if (json.get(result, "soilMax")) targetsA.soilMax = result.to<float>();
    if (json.get(result, "lightMin")) targetsA.lightMin = result.to<int>();
    if (json.get(result, "lightMax")) targetsA.lightMax = result.to<int>();
  }

  if (Firebase.RTDB.getJSON(&fbdo, "/huerto/sideB/targets")) {
    FirebaseJson &json = fbdo.jsonObject();
    FirebaseJsonData result;
    if (json.get(result, "tempMin")) targetsB.tempMin = result.to<float>();
    if (json.get(result, "tempMax")) targetsB.tempMax = result.to<float>();
    if (json.get(result, "humMin"))  targetsB.humMin  = result.to<float>();
    if (json.get(result, "humMax"))  targetsB.humMax  = result.to<float>();
    if (json.get(result, "soilMin")) targetsB.soilMin = result.to<float>();
    if (json.get(result, "soilMax")) targetsB.soilMax = result.to<float>();
    // Lado B no usa luz
  }

  // ====== CONTROL AUTOMÁTICO SIDE A ======
  Serial.println("===== CONTROL AUTOMÁTICO LADO A =====");
  bool heaterA = tempA < targetsA.tempMin;
  bool fanA    = tempA > targetsA.tempMax;
  bool pumpA   = soilA < targetsA.soilMin;
  bool lightA  = luxA < targetsA.lightMin;

  logActuator("HEATER_A", HEATER_A, heaterA);
  logActuator("FAN_A", FAN_A, fanA);
  logActuator("PUMP_A", PUMP_A, pumpA);
  logActuator("LIGHT_A", LIGHT_A, lightA);

  // ====== CONTROL AUTOMÁTICO SIDE B ======
  Serial.println("===== CONTROL AUTOMÁTICO LADO B =====");
  bool heaterB = tempB < targetsB.tempMin;
  bool fanB    = tempB > targetsB.tempMax;
  bool pumpB   = soilB < targetsB.soilMin;

  logActuator("HEATER_B", HEATER_B, heaterB);
  logActuator("FAN_B", FAN_B, fanB);
  logActuator("PUMP_B", PUMP_B, pumpB);

  // ====== CONTROL MANUAL DESDE FIREBASE ======
  if (Firebase.RTDB.getBool(&fbdo, "/huerto/sideA/actuators/fan")) logActuator("FAN_A (manual)", FAN_A, fbdo.boolData());
  if (Firebase.RTDB.getBool(&fbdo, "/huerto/sideA/actuators/heater")) logActuator("HEATER_A (manual)", HEATER_A, fbdo.boolData());
  if (Firebase.RTDB.getBool(&fbdo, "/huerto/sideA/actuators/pump")) logActuator("PUMP_A (manual)", PUMP_A, fbdo.boolData());
  if (Firebase.RTDB.getBool(&fbdo, "/huerto/sideA/actuators/light")) logActuator("LIGHT_A (manual)", LIGHT_A, fbdo.boolData());

  if (Firebase.RTDB.getBool(&fbdo, "/huerto/sideB/actuators/fan")) logActuator("FAN_B (manual)", FAN_B, fbdo.boolData());
  if (Firebase.RTDB.getBool(&fbdo, "/huerto/sideB/actuators/heater")) logActuator("HEATER_B (manual)", HEATER_B, fbdo.boolData());
  if (Firebase.RTDB.getBool(&fbdo, "/huerto/sideB/actuators/pump")) logActuator("PUMP_B (manual)", PUMP_B, fbdo.boolData());

  Serial.println(); // Separador de ciclos
  delay(5000); // cada 5s
}
