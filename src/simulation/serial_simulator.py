import random
import time
import csv
import os
from datetime import datetime, timezone

NODE_ID = "NODE_01"
BASE_LAT = 10.2000   # Reserva Pacuare, Costa Rica
BASE_LON = -83.2833
OUTPUT_FILE = "data/telemetry_log.csv"

def simular_gps():
    lat = BASE_LAT + random.randint(-100, 100) / 10000.0
    lon = BASE_LON + random.randint(-100, 100) / 10000.0
    speed = random.randint(0, 50) / 10.0
    sats = random.randint(4, 12)
    ts = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    return NODE_ID, lat, lon, speed, sats, ts

def guardar_csv(row):
    file_exists = os.path.exists(OUTPUT_FILE)
    with open(OUTPUT_FILE, "a", newline="") as f:
        writer = csv.writer(f)
        if not file_exists:
            writer.writerow(["node_id", "latitude", "longitude", "speed", "satellites", "timestamp"])
        writer.writerow(row)

if __name__ == "__main__":
    print("=== Wildlife Telemetry Node Simulator ===")
    print(f"Guardando datos en {OUTPUT_FILE}\n")

    while True:
        row = simular_gps()
        node_id, lat, lon, speed, sats, ts = row
        print(f"TX | {lat:.6f}, {lon:.6f} | {speed} km/h | {sats} sats | {ts}")
        guardar_csv(row)
        time.sleep(3)
