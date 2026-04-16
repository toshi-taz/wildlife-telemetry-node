from datetime import datetime, timezone
from . import db


class TelemetryPacket(db.Model):
    __tablename__ = "telemetry_packets"

    id = db.Column(db.Integer, primary_key=True)
    node_id = db.Column(db.String(32), nullable=False, index=True)
    camp = db.Column(db.String(64), nullable=True)
    latitude = db.Column(db.Float, nullable=False)
    longitude = db.Column(db.Float, nullable=False)
    speed = db.Column(db.Float, nullable=True)
    satellites = db.Column(db.Integer, nullable=True)
    timestamp = db.Column(db.String(32), nullable=False)
    received_at = db.Column(
        db.DateTime,
        nullable=False,
        default=lambda: datetime.now(timezone.utc),
    )

    def to_dict(self):
        return {
            "id": self.id,
            "node_id": self.node_id,
            "camp": self.camp,
            "latitude": self.latitude,
            "longitude": self.longitude,
            "speed": self.speed,
            "satellites": self.satellites,
            "timestamp": self.timestamp,
            "received_at": self.received_at.isoformat(),
        }
