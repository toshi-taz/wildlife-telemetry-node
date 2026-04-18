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

// GPS
HardwareSerial gpsSerial(2);
TinyGPSPlus gps;

// LoRa Ra-02 pines
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  2
#define LORA_FREQ  433E6

// Intervalo de envío (ms)
#define INTERVALO 10000

// =====================
// SETUP
// =====================
void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi: " + WiFi.localIP().toString());

  // LoRa
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("LoRa ERROR — verifica cableado");
  } else {
    Serial.println("LoRa OK");
  }
}

// =====================
// LOOP
// =====================
void loop() {
  // Leer GPS
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  if (gps.location.isUpdated() && gps.location.isValid()) {
    float lat = gps.location.lat();
    float lng = gps.location.lng();
    float alt = gps.altitude.meters();
    int   sat = gps.satellites.value();

    Serial.printf("LAT:%.6f LNG:%.6f ALT:%.1f SAT:%d\n", lat, lng, alt, sat);

    // Enviar por WiFi
    enviarWiFi(lat, lng, alt, sat);

    // Enviar por LoRa
    enviarLora(lat, lng, alt, sat);

    delay(INTERVALO);
  }
}

// =====================
// FUNCIONES
// =====================
void enviarWiFi(float lat, float lng, float alt, int sat) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  payload += "\"lat\":" + String(lat, 6) + ",";
  payload += "\"lng\":" + String(lng, 6) + ",";
  payload += "\"alt\":" + String(alt, 1) + ",";
  payload += "\"sat\":" + String(sat);
  payload += "}";

  int code = http.POST(payload);
  Serial.println("WiFi POST " + String(code));
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
  Serial.println("LoRa TX: " + msg);
}