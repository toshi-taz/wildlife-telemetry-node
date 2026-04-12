/*
 * Wildlife Telemetry Node
 * Simulates GPS NEO-6M + LoRa Ra-02 transmission
 * UPIEM-IPN | Alexander Toshiro Bataz López
 */

#include <Arduino.h>

// --- Configuración ---
#define NODE_ID       "NODE_01"
#define TRANSMIT_INTERVAL 10000  // ms entre transmisiones

// --- Estructura de datos GPS ---
struct GPSData {
  float latitude;
  float longitude;
  float speed;
  String timestamp;
  int satellites;
};

// --- Simulación de coordenadas (Reserva Pacuare, Costa Rica) ---
float base_lat = 10.2000;
float base_lon = -83.2833;

GPSData simularGPS() {
  GPSData gps;
  // Simula movimiento aleatorio pequeño
  gps.latitude  = base_lat + (random(-100, 100) / 10000.0);
  gps.longitude = base_lon + (random(-100, 100) / 10000.0);
  gps.speed     = random(0, 50) / 10.0;
  gps.satellites = random(4, 12);
  gps.timestamp = "2026-04-12T" + String(millis() / 1000) + "Z";
  return gps;
}

// --- Simula transmisión LoRa ---
void transmitirLoRa(GPSData gps) {
  String payload = String(NODE_ID) + "," +
                   String(gps.latitude, 6) + "," +
                   String(gps.longitude, 6) + "," +
                   String(gps.speed, 1) + "," +
                   String(gps.satellites) + "," +
                   gps.timestamp;

  Serial.println("=== TX LoRa ===");
  Serial.println(payload);
  Serial.println("===============");
}

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(0));
  Serial.println("Wildlife Telemetry Node - Iniciando...");
  Serial.println("Node ID: " NODE_ID);
}

void loop() {
  GPSData gps = simularGPS();
  transmitirLoRa(gps);
  delay(TRANSMIT_INTERVAL);
}
