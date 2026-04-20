# Wildlife Telemetry Node

Open-source conservation IoT project — ESP32 + GPS + LoRa sensor node for wildlife tracking in remote field conditions.

**Author:** Alexander Toshiro Bataz López (Toshi)  
**Program:** ISERI — IPN Zacatenco  
**Contact:** abatazl2300@alumno.ipn.mx  
**Repo:** github.com/toshi-taz/wildlife-telemetry-node

---

## Overview

Ground-based telemetry node designed for wildlife monitoring under NOM-162-SEMARNAT-2012 field constraints. No internet required in nesting zones — LoRa handles offline transmission. GPS coordinates are stored in SQLite and visualized on a Leaflet map via Flask.

Target deployment: FFCM sea turtle monitoring camp, June 2026.

---

## Hardware

| Component | Model | Interface |
|-----------|-------|-----------|
| Microcontroller | ESP32-S Dev Module | — |
| GPS | NEO-6M | UART (GPIO16 RX, GPIO17 TX) |
| LoRa | Ra-02 433MHz | SPI |
| Temp/Humidity | DHT11 | 1-Wire (GPIO4) |
| Status LED | RGB cátodo común | GPIO25/26/27 |
| Regulator | AMS1117 3.3V | — |
| Charger | TP4056 (with protection) | microUSB |
| Battery | Samsung ICR18650-26H ×3 parallel | — |

### Pin assignment

```
GPS RX    → GPIO16
GPS TX    → GPIO17
LoRa NSS  → GPIO5
LoRa RST  → GPIO14
LoRa DIO0 → GPIO2
LoRa SCK  → GPIO18
LoRa MOSI → GPIO23
LoRa MISO → GPIO19
DHT11     → GPIO4
LED R     → GPIO25
LED G     → GPIO26
LED B     → GPIO27
```

### LED status codes (field indicator)

| Color | Pattern | Meaning |
|-------|---------|---------|
| Blue | Blinking | Searching GPS fix |
| Green | Solid | Fix confirmed, transmitting |
| Orange | Blinking | WiFi connected, no GPS yet |
| Red | Blinking | Error (LoRa fail / low battery) |

---

## Stack

- **Firmware:** Arduino (ESP32), TinyGPS++, LoRa, DHT
- **Backend:** Python 3, Flask, SQLite
- **Frontend:** Leaflet.js
- **Deployment:** Render.com (dashboard)
- **Enclosure:** OpenSCAD → PETG print

---

## API

| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | /api/location | Receive GPS fix from node |
| GET | /api/locations | Last 100 locations |
| GET | / | Leaflet map |

### POST payload

```json
{
  "lat": 19.432608,
  "lng": -99.133209,
  "alt": 2240.0,
  "sat": 8
}
```

---

## Setup

```bash
cd server
pip install flask
python app.py
```

Server runs on http://0.0.0.0:5000

---

## Firmware versions

| Version | Description |
|---------|-------------|
| v0.1 | GPS UART + TinyGPS++ |
| v0.2 | WiFi + HTTP POST → Flask |
| v0.3 | LoRa Ra-02 integrated, millis() loop, GPS debug |
| v0.4 | DHT11 + LED RGB status (in progress) |

---

## Roadmap

- [x] GPS UART verified (NEO-6M → GPIO16/17)
- [x] TinyGPS++ parsing NMEA
- [x] ESP32 WiFi + HTTP POST
- [x] Flask API + SQLite
- [x] Leaflet map with auto-refresh
- [x] LoRa Ra-02 integrated — LoRa OK @ 433MHz confirmed
- [x] millis() non-blocking loop
- [x] GPS debug (charsProcessed, sentencesWithFix, satellites)
- [ ] GPS fix confirmed outdoors
- [ ] DHT11 + LED RGB (v0.4)
- [ ] AMS1117 + TP4056 on perfboard (permanent assembly)
- [ ] IP65 enclosure — OpenSCAD v0.2 with real dimensions
- [ ] Field test — FFCM camp, June 30 2026
- [ ] MoveApps contribution

---

## Field constraints

- No internet in nesting zones — LoRa handles offline transmission
- No drones permitted — ground-based nodes only
- NOM-162-SEMARNAT-2012 compliant monitoring approach
- Offline-capable, low-power, non-intrusive design
