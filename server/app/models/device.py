"""Device Model"""
from datetime import datetime
from ..extensions import db
import uuid


class Device(db.Model):
    """IoT Device"""
    __tablename__ = 'devices'

    id = db.Column(db.Integer, primary_key=True)
    device_id = db.Column(db.String(64), unique=True, nullable=False)
    name = db.Column(db.String(100), nullable=False)
    device_type = db.Column(db.String(50), nullable=False)  # desktop_dog, etc.
    description = db.Column(db.Text)

    # Status
    status = db.Column(db.String(20), default='offline')  # online, offline
    last_seen = db.Column(db.DateTime)

    # Authentication
    secret_key = db.Column(db.String(128))

    # Ownership
    user_id = db.Column(db.Integer, db.ForeignKey('users.id'))
    group_id = db.Column(db.Integer, db.ForeignKey('device_groups.id'))

    # Metadata
    firmware_version = db.Column(db.String(32))
    hardware_version = db.Column(db.String(32))
    tags = db.Column(db.JSON, default=list)

    # Current data state (merged from all reports)
    current_data = db.Column(db.JSON, default=dict)

    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    updated_at = db.Column(db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)

    # Relationships
    user = db.relationship('User', backref='devices')
    group = db.relationship('DeviceGroup', backref='devices')

    @staticmethod
    def generate_device_id():
        return str(uuid.uuid4())

    @staticmethod
    def generate_secret_key():
        return str(uuid.uuid4()).replace('-', '') + str(uuid.uuid4()).replace('-', '')

    def to_dict(self):
        return {
            'id': self.id,
            'device_id': self.device_id,
            'name': self.name,
            'device_type': self.device_type,
            'description': self.description,
            'status': self.status,
            'last_seen': self.last_seen.isoformat() if self.last_seen else None,
            'firmware_version': self.firmware_version,
            'tags': self.tags,
            'current_data': self.current_data or {},
            'created_at': self.created_at.isoformat()
        }


class DeviceGroup(db.Model):
    """Device Group"""
    __tablename__ = 'device_groups'

    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(100), nullable=False)
    description = db.Column(db.Text)
    user_id = db.Column(db.Integer, db.ForeignKey('users.id'))
    created_at = db.Column(db.DateTime, default=datetime.utcnow)

    def to_dict(self):
        return {
            'id': self.id,
            'name': self.name,
            'description': self.description,
            'device_count': len(self.devices)
        }
