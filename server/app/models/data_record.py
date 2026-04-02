"""Data Record Model"""
from datetime import datetime
from ..extensions import db


def format_float_values(data, precision=2):
    """递归格式化数据中的浮点数，解决精度问题"""
    if isinstance(data, dict):
        return {k: format_float_values(v, precision) for k, v in data.items()}
    elif isinstance(data, list):
        return [format_float_values(item, precision) for item in data]
    elif isinstance(data, float):
        return round(data, precision)
    return data


class DataRecord(db.Model):
    """Device Data Record"""
    __tablename__ = 'data_records'

    id = db.Column(db.Integer, primary_key=True)
    device_id = db.Column(db.Integer, db.ForeignKey('devices.id'), nullable=False)
    data_type = db.Column(db.String(50), nullable=False)  # sensor, status, event
    data = db.Column(db.JSON, nullable=False)
    timestamp = db.Column(db.DateTime, default=datetime.utcnow, index=True)

    device = db.relationship('Device', backref='data_records')

    def to_dict(self):
        return {
            'id': self.id,
            'device_id': self.device_id,
            'data_type': self.data_type,
            'data': format_float_values(self.data),
            'timestamp': self.timestamp.isoformat()
        }
