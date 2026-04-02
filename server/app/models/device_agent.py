"""Device Agent Binding Model"""
from datetime import datetime
from ..extensions import db


class DeviceAgent(db.Model):
    """Device-Agent Binding"""
    __tablename__ = 'device_agents'

    id = db.Column(db.Integer, primary_key=True)
    device_id = db.Column(db.Integer, db.ForeignKey('devices.id'), nullable=False)
    agent_id = db.Column(db.Integer, db.ForeignKey('agents.id'), nullable=False)
    is_active = db.Column(db.Boolean, default=True)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)

    device = db.relationship('Device', backref='device_agents')
    agent = db.relationship('Agent', backref='device_agents')

    __table_args__ = (
        db.UniqueConstraint('device_id', 'agent_id', name='uix_device_agent'),
    )

    def to_dict(self):
        return {
            'id': self.id,
            'device_id': self.device_id,
            'agent_id': self.agent_id,
            'agent_name': self.agent.name if self.agent else None,
            'is_active': self.is_active
        }
