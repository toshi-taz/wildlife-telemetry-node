#include <HardwareSerial.h>
#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <HTTPClient.h>

HardwareSerial gpsSerial(2);
TinyGPSPlus gps;

const char* ssid     = "INFINITUMA4AF";
const char* password = "yWtm9eHUfM";
const char* serverUrl = "http://192.168.1.105:5000/api/location";

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);

  WiFi.begin(ssid, password);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado: " + WiFi.localIP().toString());
}

void loop() {
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  if (gps.location.isUpdated() && gps.location.isValid()) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverUrl);
      http.addHeader("Content-Type", "application/json");

      String payload = "{";
      payload += "\"lat\":" + String(gps.location.lat(), 6) + ",";
      payload += "\"lng\":" + String(gps.location.lng(), 6) + ",";
      payload += "\"alt\":" + String(gps.altitude.meters(), 1) + ",";
      payload += "\"sat\":" + String(gps.satellites.value());
      payload += "}";

      int code = http.POST(payload);
      Serial.println("POST " + String(code) + " → " + payload);
      http.end();
    }
  }

  delay(10000); // cada 10 segundos
}