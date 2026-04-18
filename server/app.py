from flask import Flask, request, jsonify, render_template
import sqlite3, datetime

app = Flask(__name__)
DB = "telemetry.db"

def init_db():
    con = sqlite3.connect(DB)
    con.execute("""
        CREATE TABLE IF NOT EXISTS locations (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            lat       REAL NOT NULL,
            lng       REAL NOT NULL,
            alt       REAL,
            sat       INTEGER,
            timestamp TEXT DEFAULT (datetime('now','localtime'))
        )
    """)
    con.commit()
    con.close()

@app.route("/")
def index():
    return render_template("map.html")

@app.route("/api/location", methods=["POST"])
def receive_location():
    data = request.get_json()
    if not data or "lat" not in data or "lng" not in data:
        return jsonify({"error": "invalid payload"}), 400
    con = sqlite3.connect(DB)
    con.execute(
        "INSERT INTO locations (lat, lng, alt, sat) VALUES (?, ?, ?, ?)",
        (data["lat"], data["lng"], data.get("alt"), data.get("sat"))
    )
    con.commit()
    con.close()
    print(f"[{datetime.datetime.now()}] LAT={data['lat']} LNG={data['lng']} SAT={data.get('sat')}")
    return jsonify({"status": "ok"}), 201

@app.route("/api/locations", methods=["GET"])
def get_locations():
    con = sqlite3.connect(DB)
    rows = con.execute(
        "SELECT id, lat, lng, alt, sat, timestamp FROM locations ORDER BY id DESC LIMIT 100"
    ).fetchall()
    con.close()
    return jsonify([
        {"id": r[0], "lat": r[1], "lng": r[2], "alt": r[3], "sat": r[4], "timestamp": r[5]}
        for r in rows
    ])

if __name__ == "__main__":
    init_db()
    app.run(host="0.0.0.0", port=5000, debug=True)
