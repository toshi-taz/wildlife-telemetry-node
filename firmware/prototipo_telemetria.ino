// =====================================================
// Wildlife Telemetry Node v0.3
// Toshi Bataz — github.com/toshi-taz/wildlife-telemetry-node
// ESP32 + GPS NEO-6M + LoRa Ra-02 + WiFi → Flask/SQLite
// =====================================================

#include <HardwareSerial.h>
#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <LoRa.h>

// =====================
// CONFIGURACIÓN
// =====================
const char* ssid      = "INFINITUMA4AF";
const char* password  = "yWtm9eHUfM";
const char* serverUrl = "http://192.168.1.105:5000/api/location";

// GPS — GPIO16=RX2, GPIO17=TX2
HardwareSerial gpsSerial(2);
TinyGPSPlus gps;

// LoRa Ra-02 pines SPI
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  2
#define LORA_FREQ  433E6

// Intervalo de envío (ms) — no usar delay()
#define INTERVALO 10000UL

// =====================
// ESTADO GLOBAL
// =====================
bool loraOk        = false;
unsigned long lastSend  = 0;
unsigned long lastDebug = 0;

// =====================
// SETUP
// =====================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Wildlife Telemetry Node v0.3 ===");

  // GPS
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);
  Serial.println("GPS UART iniciado (GPIO16=RX, GPIO17=TX)");

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando WiFi");
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 10000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi OK: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi TIMEOUT — modo offline");
  }

  // LoRa — FLAG para no bloquear si no está conectado
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("LoRa NO disponible — modo solo GPS/WiFi");
    loraOk = false;
  } else {
    Serial.println("LoRa OK @ 433 MHz");
    loraOk = true;
  }

  Serial.println("Esperando fix GPS (sal al exterior)...");
}

// =====================
// LOOP
// =====================
void loop() {

  // 1. Leer GPS continuamente — NUNCA bloquear aquí
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  // 2. Debug periódico (cada 3s) — funciona con o sin fix
  if (millis() - lastDebug >= 3000) {
    lastDebug = millis();
    Serial.printf("[GPS] chars:%lu | fixes:%lu | sats:%d | valid:%s\n",
      gps.charsProcessed(),
      gps.sentencesWithFix(),
      gps.satellites.value(),
      gps.location.isValid() ? "SI" : "NO"
    );
    // Si no llegan chars después de 15s → problema de cableado
    if (millis() > 15000 && gps.charsProcessed() == 0) {
      Serial.println("  !! 0 chars — verifica RX/TX o swap GPIO16<->GPIO17");
    }
  }

  // 3. Enviar datos solo cuando hay fix válido y pasó el intervalo
  if (gps.location.isValid() && (millis() - lastSend >= INTERVALO)) {
    lastSend = millis();

    float lat = gps.location.lat();
    float lng  = gps.location.lng();
    float alt  = gps.altitude.meters();
    int   sat  = gps.satellites.value();
    float hdop = gps.hdop.hdop();

    Serial.printf("[FIX] LAT:%.6f LNG:%.6f ALT:%.1fm SAT:%d HDOP:%.2f\n",
      lat, lng, alt, sat, hdop);

    // Enviar por WiFi
    enviarWiFi(lat, lng, alt, sat);

    // Enviar por LoRa solo si el hardware está presente
    if (loraOk) {
      enviarLora(lat, lng, alt, sat);
    }
  }
}

// =====================
// FUNCIONESn
// =====================
void enviarWiFi(float lat, float lng, float alt, int sat) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] No conectado — skip");
    return;
  }

  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  payload += "\"lat\":"  + String(lat, 6) + ",";
  payload += "\"lng\":"  + String(lng, 6) + ",";
  payload += "\"alt\":"  + String(alt, 1) + ",";
  payload += "\"sat\":"  + String(sat);
  payload += "}";

  int code = http.POST(payload);
  Serial.println("[WiFi] POST → HTTP " + String(code));
  http.end();
}

void enviarLora(float lat, float lng, float alt, int sat) {
  String msg = String(lat, 6) + "," +
               String(lng, 6) + "," +
               String(alt, 1) + "," +
               String(sat);

  LoRa.beginPacket();
  LoRa.print(msg);
  LoRa.endPacket();
  Serial.println("[LoRa] TX: " + msg);
}