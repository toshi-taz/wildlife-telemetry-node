/*
 * ============================================================
 *  Wildlife Telemetry Node — prototipo_telemetria.ino
 * ============================================================
 *  Proyecto : wildlife-telemetry-node
 *  Repo     : github.com/toshi-taz/wildlife-telemetry-node
 *  Autor    : Toshi (ISERI, IPN Zacatenco)
 *  Version  : 0.5.0
 *  Fecha    : 2026-05
 *
 *  Cambios vs v0.4.1:
 *   [ARCH] GPS NEO-6M eliminado del diseño (Jacob / WildLabs)
 *          El WTN es un transmisor ambiental, no un tracker.
 *          Geolocalización se maneja en el receptor (Xcacel base camp).
 *   [NEW]  Deep sleep 1 hora — ESP32 duerme entre transmisiones
 *          Ciclo: wake → leer DHT22 → TX LoRa → sleep 1hr
 *   [NEW]  FET switch GPIO32 — corta alimentación a periféricos
 *          (DHT22, LoRa Ra-02, LED) durante deep sleep
 *   [FIX]  DHT_TYPE corregido a DHT22 en código y comentarios
 *          (módulos físicos son CJSL DH11, no DHT22)
 *   [FIX]  Máquina de estados simplificada sin dependencia GPS
 *          TX LoRa se ejecuta apenas LoRa OK, sin esperar fix
 *
 *  Hardware confirmado:
 *   ESP32 Dev Module
 *   LoRa  Ra-02    NSS->5  RST->14  DIO0->2  SCK->18  MOSI->23  MISO->19
 *   DHT22          DATA->GPIO4  (pull-up integrado en módulo)
 *   LED RGB        R->GPIO25  G->GPIO26  B->GPIO27  (anodo comun, 220Ω)
 *   FET switch     GPIO32 (PMOS IRLML6244, pull-up 10kΩ a 3.3V en PCB)
 *   TP4056 + AMS1117-3.3 + triple 18650 paralelo
 *
 *  LIBRERIAS NECESARIAS (instalar en Arduino IDE):
 *   1. LoRa by Sandeep Mistry
 *   2. DHT sensor library by Adafruit
 *   3. Adafruit Unified Sensor (dependencia de DHT)
 * ============================================================
 */

// ============================================================
//  LIBRERIAS
// ============================================================
#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>
#include "esp_sleep.h"

#ifdef WIFI_ENABLED
#include <WiFi.h>
#endif

// ============================================================
//  CONFIGURACION
//  Modifica esta seccion sin tocar el resto del codigo
// ============================================================

// --- Identificador del nodo ---
// Cambiar por un ID unico en cada dispositivo desplegado en campo
const char* NODE_ID = "WTN-01";

// --- WiFi ---
// Descomentá esta línea solo si necesitás WiFi en campo
// #define WIFI_ENABLED

#ifdef WIFI_ENABLED
const char* WIFI_SSID     = "TU_RED_WIFI";
const char* WIFI_PASSWORD = "TU_PASSWORD";
const unsigned long WIFI_TIMEOUT = 10000; // ms
#endif

// --- LoRa ---
const long  LORA_FREQ  = 433E6;
const int   LORA_NSS   = 5;
const int   LORA_RST   = 14;
const int   LORA_DIO0  = 2;

// --- DHT22 ---
const int   DHT_PIN  = 4;
const int   DHT_TYPE = DHT22;

// --- LED RGB (anodo comun: LOW = encendido, HIGH = apagado) ---
const int   LED_R = 25;
const int   LED_G = 26;
const int   LED_B = 27;

// --- FET switch ---
// GPIO32 LOW  → PMOS activo  → periféricos encendidos
// GPIO32 HIGH → PMOS inactivo → periféricos sin alimentación
const int   FET_SW = 32;

// --- Deep sleep ---
// 1 hora = 3600 segundos = 3,600,000,000 microsegundos
const uint64_t SLEEP_DURATION_US = 3600ULL * 1000000ULL;

// --- Intervalos (milisegundos) ---
const unsigned long DHT_INTERVAL   = 2500;  // DHT22: min 2s entre lecturas
const unsigned long PULSE_INTERVAL =  300;  // pulse LED durante TX
const unsigned long LOG_INTERVAL   = 5000;  // log serial cada 5s

// ============================================================
//  OBJETOS
// ============================================================
DHT dht(DHT_PIN, DHT_TYPE);

// ============================================================
//  ESTADOS
// ============================================================
enum SystemState {
  STATE_IDLE,         // Verde pulse — esperando ciclo TX
  STATE_TRANSMITTING, // Azul fijo  — TX LoRa en curso
  STATE_SLEEPING,     // LED apagado — deep sleep activo
  STATE_ERROR         // Rojo fijo  — fallo LoRa
};
SystemState currentState = STATE_IDLE;

// ============================================================
//  VARIABLES GLOBALES
// ============================================================
bool loraOk = false;

// Contador de paquetes — persiste en RTC memory durante deep sleep
RTC_DATA_ATTR int packetCount = 0;

// Timers
unsigned long lastDhtRead = 0;
unsigned long lastPulse   = 0;
unsigned long lastLog     = 0;

// Control LED
bool pulseState = false;

// Datos sensor
float temperature = 0.0;
float humidity    = 0.0;

// ============================================================
//  FUNCIONES LED RGB
// ============================================================
void ledOff()   { digitalWrite(LED_R,HIGH); digitalWrite(LED_G,HIGH); digitalWrite(LED_B,HIGH); }
void ledRed()   { digitalWrite(LED_R,LOW);  digitalWrite(LED_G,HIGH); digitalWrite(LED_B,HIGH); }
void ledGreen() { digitalWrite(LED_R,HIGH); digitalWrite(LED_G,LOW);  digitalWrite(LED_B,HIGH); }
void ledBlue()  { digitalWrite(LED_R,HIGH); digitalWrite(LED_G,HIGH); digitalWrite(LED_B,LOW);  }

void updateLED() {
  unsigned long now = millis();
  switch (currentState) {

    case STATE_IDLE:
      if (now - lastPulse >= PULSE_INTERVAL) {
        lastPulse  = now;
        pulseState = !pulseState;
        pulseState ? ledGreen() : ledOff();
      }
      break;

    case STATE_TRANSMITTING:
      ledBlue();
      break;

    case STATE_SLEEPING:
      ledOff();
      break;

    case STATE_ERROR:
      ledRed();
      break;
  }
}

// ============================================================
//  ENTRAR EN DEEP SLEEP
// ============================================================
void enterDeepSleep() {
  Serial.println("[Sleep] Apagando periféricos via FET...");
  Serial.printf("[Sleep] Durmiendo %.0f minutos — hasta la proxima TX\n",
                SLEEP_DURATION_US / 60000000.0);
  Serial.flush();

  // Apagar LED antes de dormir
  ledOff();
  currentState = STATE_SLEEPING;

  // Cortar alimentación a periféricos (DHT22, LoRa, LED)
  // GPIO32 HIGH → PMOS off → +3.3V_SW desconectado
  digitalWrite(FET_SW, HIGH);

  // Configurar timer de wakeup y entrar en deep sleep
  esp_sleep_enable_timer_wakeup(SLEEP_DURATION_US);
  esp_deep_sleep_start();
  // El ESP32 reinicia desde setup() al despertar
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("=== Wildlife Telemetry Node v0.5.0 ===");
  Serial.print("[Nodo] ID: "); Serial.println(NODE_ID);
  Serial.printf("[Nodo] Paquete #%d\n", packetCount + 1);

  // FET switch — encender periféricos primero
  // GPIO32 LOW → PMOS activo → +3.3V_SW disponible
  pinMode(FET_SW, OUTPUT);
  digitalWrite(FET_SW, LOW);
  delay(100); // tiempo para que suban los rieles

  // LED RGB
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  ledOff();
  Serial.println("[LED] OK");

  // DHT22
  dht.begin();
  lastDhtRead = millis();
  Serial.println("[DHT22] OK");

  // WiFi (opcional — deshabilitado por defecto en campo)
#ifdef WIFI_ENABLED
  Serial.print("[WiFi] Conectando a "); Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < WIFI_TIMEOUT) {
    delay(300);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Conectado: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n[WiFi] Fallo — modo offline");
  }
#else
  Serial.println("[WiFi] Deshabilitado (modo campo offline)");
#endif

  // LoRa
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
  if (LoRa.begin(LORA_FREQ)) {
    loraOk = true;
    Serial.println("[LoRa] OK @ 433MHz");
  } else {
    Serial.println("[LoRa] FALLO — verificar SPI/conexiones");
    currentState = STATE_ERROR;
  }

  if (currentState != STATE_ERROR) {
    currentState = STATE_IDLE;
  }

  Serial.println("[Sistema] Setup completo — loop iniciado");
  Serial.println("--------------------------------------------");
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
  unsigned long now = millis();

  // ----------------------------------------------------------
  // 1. DHT22 — leer con timer propio (min 2s entre lecturas)
  //    Leer mas rapido devuelve NaN silencioso sin error.
  // ----------------------------------------------------------
  if (now - lastDhtRead >= DHT_INTERVAL) {
    lastDhtRead = now;
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      temperature = t;
      humidity    = h;
      Serial.printf("[DHT22] T:%.1fC  H:%.1f%%\n", temperature, humidity);
    } else {
      Serial.println("[DHT22] Lectura invalida (NaN) — reintentando en 2.5s");
    }
  }

  // ----------------------------------------------------------
  // 2. LoRa TX — ejecutar una vez que LoRa OK y DHT tiene datos
  //    Sin gate de GPS — transmite apenas hay lectura válida.
  //    Después de TX exitoso entra en deep sleep 1 hora.
  // ----------------------------------------------------------
  if (loraOk && temperature != 0.0 && humidity != 0.0) {
    packetCount++;
    currentState = STATE_TRANSMITTING;
    updateLED();

    // Payload compacto key:value — compatible con receiver.py
    // Formato: ID:WTN-01,PKT:42,TMP:28.5,HUM:72.3
    String payload = String("ID:")  + NODE_ID
                   + ",PKT:"  + packetCount
                   + ",TMP:"  + String(temperature, 1)
                   + ",HUM:"  + String(humidity,    1);

    LoRa.beginPacket();
    LoRa.print(payload);
    LoRa.endPacket();

    Serial.println("[LoRa] TX -> " + payload);
    Serial.printf("[LoRa] Bytes: %d | RSSI: %d dBm\n",
                  payload.length(), LoRa.packetRssi());

    // TX completado — entrar en deep sleep 1 hora
    delay(100); // flush serial
    enterDeepSleep();
  }

  // ----------------------------------------------------------
  // 3. LED — actualizar en cada iteracion (mientras no duerme)
  // ----------------------------------------------------------
  updateLED();

  // ----------------------------------------------------------
  // 4. Log serial cada 5s (útil durante debug antes de TX)
  // ----------------------------------------------------------
  if (now - lastLog >= LOG_INTERVAL) {
    lastLog = now;
    const char* stateStr =
      currentState == STATE_IDLE         ? "IDLE"         :
      currentState == STATE_TRANSMITTING ? "TRANSMITTING" :
      currentState == STATE_SLEEPING     ? "SLEEPING"     : "ERROR";

    Serial.printf("[LOG] Estado:%s | T:%.1fC | H:%.1f%% | Pkts:%d\n",
                  stateStr, temperature, humidity, packetCount);
  }

}
