#!/usr/bin/env python3
"""
GPS node simulator — multi-threaded random walk POSTing to the telemetry API.

Usage:
    python node_sim.py [--nodes N] [--interval SEC] [--camp NAME]

Defaults:
    --nodes    3
    --interval 5
    --camp     CAMP_A
"""

import argparse
import random
import threading
import time
from datetime import datetime, timezone

import requests

API_URL = "http://localhost:5001/api/telemetry"

# Base coordinates: Sian Ka'an area, Yucatán Peninsula, MX
BASE_LAT = 20.2988
BASE_LON = -87.4578

# Max single-step drift in degrees (~11 m per 0.0001°)
STEP = 0.0005


class NodeSimulator(threading.Thread):
    """Simulates a single GPS collar node doing a correlated random walk."""

    def __init__(self, node_id: str, interval: float, camp: str, stop_event: threading.Event):
        super().__init__(daemon=True, name=node_id)
        self.node_id = node_id
        self.interval = interval
        self.camp = camp
        self.stop_event = stop_event
        # Each node starts near the base with a small random offset
        self.lat = BASE_LAT + random.uniform(-0.005, 0.005)
        self.lon = BASE_LON + random.uniform(-0.005, 0.005)

    def _walk(self):
        """Advance position by one random step."""
        self.lat += random.uniform(-STEP, STEP)
        self.lon += random.uniform(-STEP, STEP)

    def _build_payload(self):
        speed = round(random.uniform(0.0, 5.0), 2)   # km/h, slow wildlife movement
        sats = random.randint(4, 12)
        ts = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        return {
            "node_id": self.node_id,
            "camp": self.camp,
            "latitude": round(self.lat, 6),
            "longitude": round(self.lon, 6),
            "speed": speed,
            "satellites": sats,
            "timestamp": ts,
        }

    def run(self):
        print(f"[{self.node_id}] started — camp={self.camp}, interval={self.interval}s")
        while not self.stop_event.is_set():
            self._walk()
            payload = self._build_payload()
            try:
                resp = requests.post(API_URL, json=payload, timeout=5)
                status = resp.status_code
            except requests.exceptions.ConnectionError:
                status = "ERR (connection refused)"
            except requests.exceptions.Timeout:
                status = "ERR (timeout)"

            print(
                f"[{payload['node_id']}] "
                f"{payload['latitude']:.6f}, {payload['longitude']:.6f} | "
                f"{payload['speed']} km/h | {payload['satellites']} sats | "
                f"HTTP {status}"
            )
            self.stop_event.wait(self.interval)

        print(f"[{self.node_id}] stopped.")


def parse_args():
    parser = argparse.ArgumentParser(
        description="Multi-node GPS telemetry simulator"
    )
    parser.add_argument(
        "--nodes",
        type=int,
        default=3,
        metavar="N",
        help="Number of simulated nodes (default: 3)",
    )
    parser.add_argument(
        "--interval",
        type=float,
        default=5.0,
        metavar="SEC",
        help="Seconds between transmissions per node (default: 5)",
    )
    parser.add_argument(
        "--camp",
        default="CAMP_A",
        metavar="NAME",
        help="Camp / study-site label attached to every reading (default: CAMP_A)",
    )
    return parser.parse_args()


def main():
    args = parse_args()

    print("=== Wildlife Telemetry Node Simulator ===")
    print(f"  nodes={args.nodes}  interval={args.interval}s  camp={args.camp}")
    print(f"  base: {BASE_LAT}, {BASE_LON}")
    print(f"  target: {API_URL}")
    print("Press Ctrl+C to stop.\n")

    stop_event = threading.Event()
    threads = []

    for i in range(1, args.nodes + 1):
        node_id = f"NODE_{i:02d}"
        t = NodeSimulator(node_id, args.interval, args.camp, stop_event)
        threads.append(t)
        t.start()

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nShutting down…")
        stop_event.set()
        for t in threads:
            t.join(timeout=args.interval + 2)
        print("All nodes stopped.")


if __name__ == "__main__":
    main()
