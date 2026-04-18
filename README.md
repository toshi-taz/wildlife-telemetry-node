# Wildlife Telemetry Node

IoT node for real-time marine turtle tracking at FFCM nesting camps (June 2026).

Built as part of the ISERI engineering program at IPN Zacatenco, bridging smart sensor networks and wildlife conservation.

## Hardware

| Component | Status |
|-----------|--------|
| NodeMCU ESP-32S | Active |
| GPS u-blox NEO-6M (GY-GPS6MV2) | UART verified |
| LoRa Ra-02 | Pending |
| AMS1117 3.3V regulator | Pending |
| 18650 battery holder | Pending |
| IP65 enclosure (PETG) | Design phase |

## Architecture

```
GPS NEO-6M → ESP32 (UART2 GPIO16/17)
ESP32 → WiFi → Flask API → SQLite
Flask → Leaflet.js map (auto-refresh 10s)
```

## Stack

- **Firmware:** Arduino (ESP32), TinyGPS++, HardwareSerial
- **Backend:** Python 3, Flask, SQLite
- **Frontend:** Leaflet.js, OpenStreetMap
- **Deployment target:** Render.com (field server)

## Wiring

| GPS Pin | ESP32 Pin |
|---------|-----------|
| VCC | 3V3 |
| GND | GND |
| TX | GPIO16 (RX2) |
| RX | GPIO17 (TX2) |

## API

| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | /api/location | Receive GPS fix from ESP32 |
| GET | /api/locations | Retrieve last 100 locations |
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

## Setup

```bash
cd server
pip install flask
python app.py
```

Server runs on http://0.0.0.0:5000

## Roadmap

- [x] GPS UART verified (NEO-6M → GPIO16/17)
- [x] TinyGPS++ parsing NMEA
- [x] ESP32 WiFi + HTTP POST
- [x] Flask API + SQLite
- [x] Leaflet map with auto-refresh
- [ ] GPS fix confirmed outdoors
- [ ] LoRa Ra-02 integration
- [ ] IP65 enclosure (PETG, OpenSCAD)
- [ ] Field test at FFCM camp — June 2026
- [ ] MoveApps contribution

## Field constraints

- No internet in nesting zones — LoRa mesh planned for offline operation
- No drones permitted — ground-based nodes only
- NOM-162-SEMARNAT-2012 compliant monitoring approach

## Author

Alexander Toshiro Bataz López (Toshi)  
ISERI — IPN Zacatenco  
abatazl2300@alumno.ipn.mx  
github.com/toshi-taz
