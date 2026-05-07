# Wildlife Telemetry Node (WTN)

Fixed sensor node for sea turtle nest monitoring at FFCM Campamentos Tortugueros, Quintana Roo, Mexico.  
Built with ESP32 + LoRa 433 MHz. No GPS — nodes are stationary; coordinates are registered once at deployment.

## Overview

Each WTN is deployed at a fixed location on a nesting beach and transmits environmental readings (temperature, humidity) over LoRa to a base station connected to a laptop or Raspberry Pi running `receiver.py`. Data is stored as CSV and fed into the wildlife-telemetry-pipeline for analysis.

## Hardware

| Component | Part |
|---|---|
| Microcontroller | ESP32 (38-pin DevKit) |
| Radio | Ra-02 / SX1278 — 433 MHz LoRa |
| Temperature & Humidity | DHT22 |
| Power | Samsung ICR18650-26H ×3 parallel |

## Payload Format

Nodes transmit a compact key:value string over LoRa:

```
ID:WTN-01,PKT:42,TMP:28.5,HUM:65.2
```

| Field | Description |
|---|---|
| `ID` | Node identifier (e.g. `WTN-01`) |
| `PKT` | Packet counter |
| `TMP` | Temperature (°C) |
| `HUM` | Relative humidity (%) |

No GPS fields — position is fixed and resolved at the receiver from the node ID lookup table in `receiver.py`.

## Firmware

`firmware/telemetry_node.ino` — Arduino sketch for ESP32. Reads DHT22, builds payload string, and transmits via LoRa every N seconds.

## Repository Structure

```
wildlife-telemetry-node/
├── firmware/
│   └── telemetry_node.ino   # ESP32 Arduino sketch
├── enclosure/               # 3D-printable enclosure (OpenSCAD)
├── src/
│   └── simulation/          # Serial simulator for offline testing
├── server/
│   └── app.py               # Local Flask dashboard (live readings)
└── docs/
```

## Related Repos

- [wildlife-telemetry-pipeline](https://github.com/toshi-taz/wildlife-telemetry-pipeline) — `receiver.py` + data processing + MoveApps integration
- [track-classifier](https://github.com/toshi-taz/track-classifier) — Gemini Vision Flask app for identifying sea turtle tracks from photos

## Author

Alexander Toshiro Bataz López  
Ingeniería en Sistemas Energéticos y Redes Inteligentes — UPIEM–IPN  
Conservation Technology | Wildlife Telemetry | IoT Sensor Networks
