"""Device Type Template Model"""
from datetime import datetime
from ..extensions import db


class DeviceTemplate(db.Model):
    """Device Type Template"""
    __tablename__ = 'device_templates'

    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(50), unique=True, nullable=False)
    display_name = db.Column(db.String(100), nullable=False)
    description = db.Column(db.Text)
    icon = db.Column(db.String(50))

    # Owner (None = system default, can be deleted by anyone)
    user_id = db.Column(db.Integer, db.ForeignKey('users.id'), nullable=True)

    # Data points definition (upload data format)
    # Example: [{"name": "temperature", "type": "float", "unit": "°C", "min": -40, "max": 80}]
    data_points = db.Column(db.JSON, default=list)

    # Commands definition (download commands)
    # Example: [{"name": "set_led", "params": [{"name": "color", "type": "string"}]}]
    commands = db.Column(db.JSON, default=list)

    # Config schema (device settings)
    config_schema = db.Column(db.JSON, default=dict)

    # Config items template (custom config items)
    # Example: [{"key": "threshold", "label": "阈值", "type": "float", "default_value": "25"}]
    config_items = db.Column(db.JSON, default=list)

    # Communication settings
    comm_settings = db.Column(db.JSON, default=lambda: {
        'heartbeat_interval': 60,
        'data_interval': 300,
        'offline_timeout': 180,
        'protocol': 'http'
    })

    # Hardware info
    hardware_platform = db.Column(db.String(50), default='esp32')
    firmware_version = db.Column(db.String(20))

    is_active = db.Column(db.Boolean, default=True)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    updated_at = db.Column(db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)

    def to_dict(self):
        return {
            'id': self.id,
            'name': self.name,
            'display_name': self.display_name,
            'description': self.description,
            'icon': self.icon,
            'user_id': self.user_id,
            'data_points': self.data_points or [],
            'commands': self.commands or [],
            'config_schema': self.config_schema or {},
            'config_items': self.config_items or [],
            'comm_settings': self.comm_settings or {},
            'hardware_platform': self.hardware_platform,
            'firmware_version': self.firmware_version,
            'is_active': self.is_active,
            'created_at': self.created_at.isoformat() if self.created_at else None
        }
