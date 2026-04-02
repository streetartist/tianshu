"""Device Configuration Model"""
from datetime import datetime
from ..extensions import db


class DeviceConfig(db.Model):
    """Device Configuration"""
    __tablename__ = 'device_configs'

    id = db.Column(db.Integer, primary_key=True)
    device_id = db.Column(db.Integer, db.ForeignKey('devices.id'), nullable=False)
    key = db.Column(db.String(100), nullable=False)
    value = db.Column(db.JSON)
    version = db.Column(db.Integer, default=1)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    updated_at = db.Column(db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)

    device = db.relationship('Device', backref='configs')

    __table_args__ = (
        db.UniqueConstraint('device_id', 'key', name='uix_device_config'),
    )

    def to_dict(self):
        return {
            'id': self.id,
            'key': self.key,
            'value': self.value,
            'version': self.version,
            'updated_at': self.updated_at.isoformat()
        }
