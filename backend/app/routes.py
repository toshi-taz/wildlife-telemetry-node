from flask import Blueprint, request, jsonify, render_template
from . import db
from .models import TelemetryPacket

bp = Blueprint("api", __name__)

REQUIRED_FIELDS = ("node_id", "latitude", "longitude", "timestamp")


# ---------------------------------------------------------------------------
# Dashboard
# ---------------------------------------------------------------------------

@bp.route("/")
def index():
    return render_template("index.html")


# ---------------------------------------------------------------------------
# Health
# ---------------------------------------------------------------------------

@bp.route("/api/health")
def health():
    return jsonify({"status": "ok"}), 200


# ---------------------------------------------------------------------------
# Ingest
# ---------------------------------------------------------------------------

@bp.route("/api/telemetry", methods=["POST"])
def ingest():
    data = request.get_json(silent=True)
    if not data:
        return jsonify({"error": "JSON body required"}), 400

    missing = [f for f in REQUIRED_FIELDS if f not in data]
    if missing:
        return jsonify({"error": f"Missing fields: {missing}"}), 422

    packet = TelemetryPacket(
        node_id=data["node_id"],
        camp=data.get("camp"),
        latitude=float(data["latitude"]),
        longitude=float(data["longitude"]),
        speed=data.get("speed"),
        satellites=data.get("satellites"),
        timestamp=data["timestamp"],
    )
    db.session.add(packet)
    db.session.commit()
    return jsonify({"status": "stored", "id": packet.id}), 201


# ---------------------------------------------------------------------------
# Live feed — last N packets per node (default: 50 points per node)
# ---------------------------------------------------------------------------

@bp.route("/api/telemetry/live")
def live():
    limit = min(int(request.args.get("limit", 50)), 500)
    camp = request.args.get("camp")

    # Collect distinct node IDs
    query = db.session.query(TelemetryPacket.node_id).distinct()
    if camp:
        query = query.filter(TelemetryPacket.camp == camp)
    node_ids = [row[0] for row in query]

    result = {}
    for nid in node_ids:
        q = (
            TelemetryPacket.query
            .filter(TelemetryPacket.node_id == nid)
        )
        if camp:
            q = q.filter(TelemetryPacket.camp == camp)
        rows = (
            q.order_by(TelemetryPacket.id.desc())
            .limit(limit)
            .all()
        )
        # Return in chronological order
        result[nid] = [r.to_dict() for r in reversed(rows)]

    return jsonify(result), 200


# ---------------------------------------------------------------------------
# Node list
# ---------------------------------------------------------------------------

@bp.route("/api/nodes")
def nodes():
    rows = (
        db.session.query(
            TelemetryPacket.node_id,
            TelemetryPacket.camp,
            db.func.count(TelemetryPacket.id).label("packet_count"),
            db.func.max(TelemetryPacket.timestamp).label("last_seen"),
        )
        .group_by(TelemetryPacket.node_id)
        .all()
    )
    return jsonify([
        {
            "node_id": r.node_id,
            "camp": r.camp,
            "packet_count": r.packet_count,
            "last_seen": r.last_seen,
        }
        for r in rows
    ]), 200
